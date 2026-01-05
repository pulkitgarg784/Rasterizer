#include "mesh.h"

#include <fstream>
#include <iostream>
#include <sstream>

Mesh::Mesh(std::vector<vec3> verts, std::vector<int> faces, std::vector<vec3> norms, std::vector<int> face_norms, std::vector<vec2> uvs, std::vector<int> face_uvs) 
    : vertices(verts), face_vertices(faces), normals(norms), face_normals(face_norms), uvs(uvs), face_uvs(face_uvs) {}

Mesh::Mesh(const std::string filename) {
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
    } else if (prefix == "vt") {
      vec2 uv;
      iss >> uv[0] >> uv[1];
      uvs.push_back(uv);
    } else if (prefix == "f") {
      std::string vertex_data;
      std::vector<int> face_indices;
      std::vector<int> face_normal_indices;
      std::vector<int> face_uv_indices;

      while (iss >> vertex_data) {
        int v_idx = 0;
        int vn_idx = 0;
        int vt_idx = 0;
        // Parse the vertex index (before first '/' or entire string)
        size_t slash_pos = vertex_data.find('/');
        if (slash_pos == std::string::npos) {
          v_idx = std::stoi(vertex_data);
        } else {
          v_idx = std::stoi(vertex_data.substr(0, slash_pos));
          size_t second_slash = vertex_data.find('/', slash_pos + 1);
          if (second_slash != std::string::npos) {
            std::string vt_part = vertex_data.substr(slash_pos + 1, second_slash - slash_pos - 1);
            if (!vt_part.empty()) vt_idx = std::stoi(vt_part);
            
            std::string vn_part = vertex_data.substr(second_slash + 1);
            if (!vn_part.empty()) vn_idx = std::stoi(vn_part);
          } else {
             std::string vt_part = vertex_data.substr(slash_pos + 1);
             if (!vt_part.empty()) vt_idx = std::stoi(vt_part);
          }
        }
        // OBJ indices are 1-based, convert to 0-based
        // Handle negative indices (relative to end)
        if (v_idx < 0) v_idx = vertices.size() + v_idx + 1;
        face_indices.push_back(v_idx - 1);

        if (vn_idx != 0) {
          if (vn_idx < 0) vn_idx = normals.size() + vn_idx + 1;
          face_normal_indices.push_back(vn_idx - 1);
        }
        
        if (vt_idx != 0) {
            if (vt_idx < 0) vt_idx = uvs.size() + vt_idx + 1;
            face_uv_indices.push_back(vt_idx - 1);
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
        
        if (!face_uv_indices.empty()) {
            face_uvs.push_back(face_uv_indices[0]);
            face_uvs.push_back(face_uv_indices[i]);
            face_uvs.push_back(face_uv_indices[i + 1]);
        }
      }
    }
  }
}

void Mesh::normalize() {
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

int Mesh::nverts() const { return vertices.size(); }

int Mesh::nfaces() const { return face_vertices.size() / 3; }

vec3 Mesh::vertex(const int i) const { return vertices[i]; }

vec3 Mesh::vertex(const int iface, const int nthvertex) const {
  return vertices[face_vertices[iface * 3 + nthvertex]];
}

vec3 Mesh::normal(const int iface, const int nthvertex) const {
  return normals[face_normals[iface * 3 + nthvertex]];
}

vec2 Mesh::uv(const int iface, const int nthvertex) const {
  if (face_uvs.empty()) return {0, 0};
  return uvs[face_uvs[iface * 3 + nthvertex]];
}

void Mesh::load_texture(const std::string filename) {
    if (diffuse_map.read_tga_file(filename)) {
        diffuse_map.flip_vertically();
        has_texture = true;
    }
}

void Mesh::load_normal_map(const std::string filename) {
    if (normal_map.read_tga_file(filename)) {
        normal_map.flip_vertically();
        has_normal_map = true;
    }
}

void Mesh::load_specular_map(const std::string filename) {
    if (specular_map.read_tga_file(filename)) {
        specular_map.flip_vertically();
        has_specular_map = true;
    }
}

TGAColor Mesh::diffuse(vec2 uv) const {
    if (!has_texture) return {255, 255, 255, 255};
    return diffuse_map.get(uv[0] * diffuse_map.width(), uv[1] * diffuse_map.height());
}

float Mesh::specular(vec2 uv) const {
    if (!has_specular_map) return 1.0f;
    return specular_map.get(uv[0] * specular_map.width(), uv[1] * specular_map.height())[0] / 1.0f;
}

vec3 Mesh::normal(vec2 uv) const {
    if (!has_normal_map) return {0, 0, 0}; // Should handle this case in shader
    TGAColor c = normal_map.get(uv[0] * normal_map.width(), uv[1] * normal_map.height());
    vec3 res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = (double)c[i] / 255. * 2. - 1.;
    return res;
}
