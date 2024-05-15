#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char* filename) {
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
            parseTextureCoord(iss);
        } else if (line.substr(0, 3) == "vn ") {
            parseNormal(iss);
        } else if (line.substr(0, 2) == "f ") {
            parseFace(iss);
        }
    }

    std::cerr << "# v# " << verts_.size() << " f# " << faces_.size() << " vt# " << texture_.size() << " vn# " << normal_.size() << std::endl;
}

void Model::parseVertex(std::istringstream& iss) {
    Vec3f v;
    char trash;
    iss >> trash;  // Skip the 'v' character
    for (int i = 0; i < 3; ++i) iss >> v[i];
    verts_.push_back(v);
}

void Model::parseTextureCoord(std::istringstream& iss) {
    Vec2f vt;
    char trash;
    iss >> trash >> trash;  // Skip the "vt" prefix
    for (int i = 0; i < 2; ++i) iss >> vt[i];
    texture_.push_back(vt);
}

void Model::parseNormal(std::istringstream& iss) {
    Vec3f vn;
    char trash;
    iss >> trash >> trash;  // Skip the "vn" prefix
    for (int i = 0; i < 3; ++i) iss >> vn[i];
    normal_.push_back(vn);
}

void Model::parseFace(std::istringstream& iss) {
    std::vector<int> f, vt_indices, vn_indices;
    int idx, vt_idx, vn_idx;
    char trash;
    iss >> trash;  // Skip the 'f' character
    while (iss >> idx >> trash >> vt_idx >> trash >> vn_idx) {
        idx--;  // Convert from 1-based to 0-based
        vt_idx--;
        vn_idx--;
        f.push_back(idx);
        vt_indices.push_back(vt_idx);
        vn_indices.push_back(vn_idx);
    }
    faces_.push_back(f);
    texture_index_.push_back(vt_indices);
    normal_index_.push_back(vn_indices);
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

Vec3f Model::normal(int i) {
    return normal_[i];
}

int Model::normal_index(int face_idx, int vert_idx) {
    return normal_index_[face_idx][vert_idx];
}