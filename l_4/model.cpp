#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) : verts_(), faces_(), texture_(), texture_index_() {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        if (line.substr(0, 2) == "v ") {
            parseVertex(iss);
        } else if (line.substr(0, 3) == "vt ") {
            parseTexture(iss);
        } else if (line.substr(0, 2) == "f ") {
            parseFace(iss);
        }
    }

    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << std::endl;
}

void Model::parseVertex(std::istringstream& iss) {
    Vec3f v;
    iss.ignore(2); // Skip v
    for (int i = 0; i < 3; i++) iss >> v.raw[i];
    verts_.push_back(v);
}

void Model::parseTexture(std::istringstream& iss) {
    Vec2f vt;
    iss.ignore(3); // Skip vt
    for (int i = 0; i < 2; i++) iss >> vt.raw[i];
    texture_.push_back(vt);
}

void Model::parseFace(std::istringstream& iss) {
    iss.ignore(2); // Skip f
    std::vector<int> f;
    std::vector<int> vt_indices;
    int idx, vt_idx, n_idx;
    char trash;
    while (iss >> idx >> trash >> vt_idx >> trash >> n_idx) {
        f.push_back(idx - 1);
        vt_indices.push_back(vt_idx - 1);
    }
    faces_.push_back(f);
    texture_index_.push_back(vt_indices);
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec2f Model::texture(int i) {
    return texture_[i];
}

int Model::texture_index(int face_idx, int vert_idx) {
    return texture_index_[face_idx][vert_idx];
}