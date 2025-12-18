#include <fstream>
#include <sstream>
#include "model.h"
#include <iostream>

Model::Model(const std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            vec3 v;
            iss >> v[0] >> v[1] >> v[2];
            vertices.push_back(v);
        } else if (prefix == "f") {
            std::string vertex_data;
            std::vector<int> face_indices;
            
            while (iss >> vertex_data) {
                int v_idx = 0;
                // Parse the vertex index (before first '/' or entire string)
                size_t slash_pos = vertex_data.find('/');
                if (slash_pos == std::string::npos) {
                    v_idx = std::stoi(vertex_data);
                } else {
                    v_idx = std::stoi(vertex_data.substr(0, slash_pos));
                }
                // OBJ indices are 1-based, convert to 0-based
                // Handle negative indices (relative to end)
                if (v_idx < 0) {
                    v_idx = vertices.size() + v_idx + 1;
                }
                face_indices.push_back(v_idx - 1);
            }
            
            // Triangulate faces with more than 3 vertices (fan triangulation)
            for (size_t i = 1; i + 1 < face_indices.size(); i++) {
                face_vertices.push_back(face_indices[0]);
                face_vertices.push_back(face_indices[i]);
                face_vertices.push_back(face_indices[i + 1]);
            }
        }
    }
    std::cerr << "# v# " << nverts() << " f# " << nfaces() << std::endl;
}

void Model::normalize() {
    if (vertices.empty()) return;
    
    vec3 min_v = vertices[0];
    vec3 max_v = vertices[0];
    
    for (const auto& v : vertices) {
        for (int i = 0; i < 3; i++) {
            if (v[i] < min_v[i]) min_v[i] = v[i];
            if (v[i] > max_v[i]) max_v[i] = v[i];
        }
    }
    
    vec3 center;
    float max_extent = 0;
    for (int i = 0; i < 3; i++) {
        center[i] = (min_v[i] + max_v[i]) / 2.0f;
        float extent = max_v[i] - min_v[i];
        if (extent > max_extent) max_extent = extent;
    }
    
    float scale = (max_extent > 0) ? 2.0f / max_extent : 1.0f;
    
    for (auto& v : vertices) {
        for (int i = 0; i < 3; i++) {
            v[i] = (v[i] - center[i]) * scale;
        }
    }
}


int Model::nverts() const { return vertices.size(); }

int Model::nfaces() const { return face_vertices.size() / 3; }

vec3 Model::vertex(const int i) const { return vertices[i]; }

vec3 Model::vertex(const int iface, const int nthvertex) const {
    return vertices[face_vertices[iface * 3 + nthvertex]];
}