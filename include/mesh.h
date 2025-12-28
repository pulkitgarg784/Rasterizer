#ifndef RASTERIZER_MESH_H
#define RASTERIZER_MESH_H

#include <vector>
#include "tgaimage.h"
#include "vec.h"

class Mesh {
  std::vector<vec3> vertices = {};
  std::vector<int> face_vertices = {};
  std::vector<vec3> normals = {};
  std::vector<int> face_normals = {};
  std::vector<vec2> uvs = {};
  std::vector<int> face_uvs = {};
  TGAImage diffuse_map = {};
  TGAImage normal_map = {};
  bool has_texture = false;
  bool has_normal_map = false;

 public:
  Mesh(const std::string filename);
  Mesh(std::vector<vec3> verts, std::vector<int> faces, std::vector<vec3> norms, std::vector<int> face_norms, std::vector<vec2> uvs = {}, std::vector<int> face_uvs = {});
  int nverts() const;
  int nfaces() const;
  vec3 vertex(const int i) const;
  vec3 vertex(const int iface, const int nthvertex) const;
  vec3 normal(const int iface, const int nthvertex) const;
  vec2 uv(const int iface, const int nthvertex) const;
  void normalize();
  
  void load_texture(const std::string filename);
  void load_normal_map(const std::string filename);
  TGAColor diffuse(vec2 uv) const;
  vec3 normal(vec2 uv) const;
  bool hasNormalMap() const { return has_normal_map; }
};

#endif  // RASTERIZER_MESH_H