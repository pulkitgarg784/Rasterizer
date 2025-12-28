//
// Created by Pulkit Garg on 2025-12-17.
//

#ifndef RASTERIZER_MODEL_H
#define RASTERIZER_MODEL_H

#include <vector>

#include "vec.h"
class Model {
  std::vector<vec3> vertices = {};
  std::vector<int> face_vertices = {};
  std::vector<vec3> normals = {};    // array of normal vectors
  std::vector<int> face_normals = {};

 public:
  Model(const std::string filename);
  int nverts() const;
  int nfaces() const;
  vec3 vertex(const int i) const;
  vec3 vertex(const int iface, const int nthvertex) const;
  vec3 normal(const int iface, const int nthvertex) const;
  void normalize();
};

#endif  // RASTERIZER_MODEL_H