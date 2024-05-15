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

Vec3f barycentric(const Vec3f& A, const Vec3f& B, const Vec3f& C, const Vec3f& P) {
    Vec3f v0 = B - A;
    Vec3f v1 = C - A;
    Vec3f v2 = P - A;
    
    float d00 = v0 * v0;
    float d01 = v0 * v1;
    float d11 = v1 * v1;
    float d20 = v2 * v0;
    float d21 = v2 * v1;
    
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    
    return Vec3f(u, v, w);
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
            Vec3f baryCoords = barycentric(pts[0], pts[1], pts[2], Vec3f(P.x, P.y, 0));
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

	Vec3f light_dir(0,0,-1);

    float *zbuffer = new float[width*height];

    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = std::numeric_limits<int>::min();
    }

    std::vector<std::vector<Vec2f>> all_tex_coords;

	for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec2f tex_coords[3];

        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec3f((v.x + 1.) * width / 2., (v.y + 1.) * height / 2., v.z);
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

    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

