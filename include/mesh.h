#ifndef RASTERIZER_MESH_H
#define RASTERIZER_MESH_H

#include <vector>

#include "vec.h"
class Mesh {
  std::vector<vec3> vertices = {};
  std::vector<int> face_vertices = {};
  std::vector<vec3> normals = {};
  std::vector<int> face_normals = {};

 public:
  Mesh(const std::string filename);
  Mesh(std::vector<vec3> verts, std::vector<int> faces, std::vector<vec3> norms, std::vector<int> face_norms);
  int nverts() const;
  int nfaces() const;
  vec3 vertex(const int i) const;
  vec3 vertex(const int iface, const int nthvertex) const;
  vec3 normal(const int iface, const int nthvertex) const;
  void normalize();
};

#endif  // RASTERIZER_MESH_H