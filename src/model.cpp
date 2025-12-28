#include "model.h"

#include <fstream>
#include <iostream>
#include <sstream>

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
    } else if (prefix == "vn") {
      vec3 n;
      iss >> n[0] >> n[1] >> n[2];
      normals.push_back(n);
    } else if (prefix == "f") {
      std::string vertex_data;
      std::vector<int> face_indices;
      std::vector<int> face_normal_indices;

      while (iss >> vertex_data) {
        int v_idx = 0;
        int vn_idx = 0;
        // Parse the vertex index (before first '/' or entire string)
        size_t slash_pos = vertex_data.find('/');
        if (slash_pos == std::string::npos) {
          v_idx = std::stoi(vertex_data);
        } else {
          v_idx = std::stoi(vertex_data.substr(0, slash_pos));
          size_t second_slash = vertex_data.find('/', slash_pos + 1);
          if (second_slash != std::string::npos) {
            std::string vn_part = vertex_data.substr(second_slash + 1);
            if (!vn_part.empty()) {
              vn_idx = std::stoi(vn_part);
            }
          }
        }
        // OBJ indices are 1-based, convert to 0-based
        // Handle negative indices (relative to end)
        if (v_idx < 0) {
          v_idx = vertices.size() + v_idx + 1;
        }
        face_indices.push_back(v_idx - 1);

        if (vn_idx != 0) {
          if (vn_idx < 0) vn_idx = normals.size() + vn_idx + 1;
          face_normal_indices.push_back(vn_idx - 1);
        }
      }

      // Triangulate faces with more than 3 vertices (fan triangulation)
      for (size_t i = 1; i + 1 < face_indices.size(); i++) {
        face_vertices.push_back(face_indices[0]);
        face_vertices.push_back(face_indices[i]);
        face_vertices.push_back(face_indices[i + 1]);

        if (!face_normal_indices.empty()) {
          face_normals.push_back(face_normal_indices[0]);
          face_normals.push_back(face_normal_indices[i]);
          face_normals.push_back(face_normal_indices[i + 1]);
        }
      }
    }
  }
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

vec3 Model::normal(const int iface, const int nthvertex) const {
  return normals[face_normals[iface * 3 + nthvertex]];
}