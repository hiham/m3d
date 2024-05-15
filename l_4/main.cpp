#include <vector>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;
const int depth  = 255;
const Vec3f camera(0,0,3);

Vec3f matrix2vector(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

Matrix vector2matrix(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

void line(Vec2i pt1, Vec2i pt2, TGAImage &image, TGAColor color) {
    bool steep = false;
    if (std::abs(pt1.x-pt2.x)<std::abs(pt1.y-pt2.y)) {
        std::swap(pt1.x, pt1.y);
        std::swap(pt2.x, pt2.y);
        steep = true;
    }
    if (pt1.x>pt2.x) {
        std::swap(pt1.x, pt2.x);
        std::swap(pt1.y, pt2.y);
    }

    for (int x=pt1.x; x<=pt2.x; x++) {
        float t = (x-pt1.x)/(float)(pt2.x-pt1.x);
        int y = pt1.y*(1.-t) + pt2.y*t;
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
    }
}

Vec3f barycentre(const Vec3f& A, const Vec3f& B, const Vec3f& C, const Vec3f& P) {
    float aireABC = (float)((B.x - A.x) * (C.y - A.y) - (C.x - A.x) * (B.y - A.y));
    float alpha = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / aireABC;
    float beta = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / aireABC;
    float gamma = 1.0f - alpha - beta;

    return Vec3f(alpha, beta, gamma);
}

Vec2f interpolateTextureCoordinates(const Vec2f &tc0, const Vec2f &tc1, const Vec2f &tc2, const Vec3f &baryCoords) {
    return tc0 * baryCoords.x + tc1 * baryCoords.y + tc2 * baryCoords.z;
}
 
void triangle(Vec3f *pts, Vec2f *tex_coords, float *zbuffer, TGAImage &image, TGAImage &texture, float intensity, int width) {
    Vec2i boxMin, boxMax;
    for (int i = 0; i < 3; i++) {
        boxMin.x = std::min(boxMin.x, static_cast<int>(pts[i].x));
        boxMax.x = std::max(boxMax.x, static_cast<int>(pts[i].x));
        boxMin.y = std::min(boxMin.y, static_cast<int>(pts[i].y));
        boxMax.y = std::max(boxMax.y, static_cast<int>(pts[i].y));
    }
    Vec3f P;
    for (P.x = boxMin.x; P.x <= boxMax.x; P.x++) {
        for (P.y = boxMin.y; P.y <= boxMax.y; P.y++) {
            Vec3f baryCoords = barycentre(pts[0], pts[1], pts[2], Vec3f(P.x, P.y, 0));
            if (baryCoords.x < 0 || baryCoords.y < 0 || baryCoords.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i].z * baryCoords.raw[i];
            int idx = int(P.x + P.y * width);
            if (zbuffer[idx] < P.z) {
                zbuffer[idx] = P.z;
                Vec2f texCoord = interpolateTextureCoordinates(tex_coords[0], tex_coords[1], tex_coords[2], baryCoords);
                int tex_x = int(texCoord.x * texture.get_width());
                int tex_y = int(texCoord.y * texture.get_height());

                TGAColor color = texture.get(texture.get_width() - tex_x, texture.get_height() - tex_y);
                color = color * intensity;
                image.set(P.x, P.y, color);
            }
        }
    }
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }


    TGAImage image(width, height, TGAImage::RGB);

    TGAImage texture; 
    texture.read_tga_file("obj/african_head_diffuse.tga");

	Vec3f light_dir(0,0,-1); // define light_dir

    float *zbuffer = new float[width*height];

    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    std::vector<std::vector<Vec2f>> all_tex_coords;

    Matrix Projection = Matrix::identity(4);
    Matrix ViewPort   = viewport(width/8, height/8, width*3/4, height*3/4);
    Projection[3][2] = -1.f/camera.z;
    

	for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec2f tex_coords[3];

        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = matrix2vector(ViewPort*Projection*vector2matrix(v)); 
            world_coords[j] = v;

            int vt_index = model->texture_index(i, j);
            tex_coords[j] = model->texture(vt_index);

        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords, tex_coords, zbuffer, image, texture, intensity, image.get_width());
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
