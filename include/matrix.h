#ifndef RASTERIZER_MAT_H
#define RASTERIZER_MAT_H

#include <cassert>
#include <cmath>
#include <iostream>

#include "vec.h"

template <int N> struct dt;

template <int Rows, int Cols>
struct mat {
  vec<Cols> rows[Rows] = {{}};

  // Access operators
  vec<Cols>& operator[](int idx) {
    assert(idx >= 0 && idx < Rows);
    return rows[idx];
  }
  const vec<Cols>& operator[](int idx) const {
    assert(idx >= 0 && idx < Rows);
    return rows[idx];
  }

  // Element access
  double& operator()(int row, int col) { return rows[row][col]; }
  double operator()(int row, int col) const { return rows[row][col]; }

  // Identity matrix
  static mat<Rows, Cols> identity() {
    static_assert(Rows == Cols, "Identity only defined for square matrices");
    mat<Rows, Cols> m;
    for (int i = 0; i < Rows; ++i) m[i][i] = 1.0;
    return m;
  }

  // Determinant
  double det() const {
    static_assert(Rows == Cols, "Determinant only defined for square matrices");
    return dt<Cols>::det(*this);
  }

  // Cofactor
  double cofactor(int row, int col) const {
    static_assert(Rows == Cols, "Cofactor only defined for square matrices");
    mat<Rows - 1, Cols - 1> submatrix;
    for (int i = Rows - 1; i--;)
      for (int j = Cols - 1; j--; submatrix[i][j] = rows[i + int(i >= row)][j + int(j >= col)]);
    return submatrix.det() * ((row + col) % 2 ? -1 : 1);
  }

  // Inverse transpose
  mat<Rows, Cols> invert_transpose() const {
    static_assert(Rows == Cols, "Inverse only defined for square matrices");
    mat<Rows, Cols> adj;
    for (int i = Rows; i--;)
      for (int j = Cols; j--; adj[i][j] = cofactor(i, j));
    return adj / dot(adj[0], rows[0]);
  }

  // Inverse
  mat<Rows, Cols> invert() const {
    return invert_transpose().transpose();
  }

  // Transpose
  mat<Cols, Rows> transpose() const {
    mat<Cols, Rows> result;
    for (int i = Cols; i--;)
      for (int j = Rows; j--; result[i][j] = rows[j][i]);
    return result;
  }
};

// Determinant helper
template <int N>
struct dt {
  static double det(const mat<N, N>& src) {
    double result = 0;
    for (int i = N; i--; result += src[0][i] * src.cofactor(0, i));
    return result;
  }
};

template <>
struct dt<1> {
  static double det(const mat<1, 1>& src) {
    return src[0][0];
  }
};

using mat2 = mat<2, 2>;
using mat3 = mat<3, 3>;
using mat4 = mat<4, 4>;

template <int Rows, int Cols>
vec<Rows> operator*(const mat<Rows, Cols>& m, const vec<Cols>& v) {
  vec<Rows> result;
  for (int i = Rows; i--; result[i] = dot(m[i], v));
  return result;
}

template <int Rows, int Cols>
vec<Cols> operator*(const vec<Rows>& v, const mat<Rows, Cols>& m) {
  return (mat<1, Rows>{{v}} * m)[0];
}

template <int R1, int C1, int C2>
mat<R1, C2> operator*(const mat<R1, C1>& a, const mat<C1, C2>& b) {
  mat<R1, C2> result;
  for (int i = R1; i--;)
    for (int j = C2; j--;)
      for (int k = C1; k--; result[i][j] += a[i][k] * b[k][j]);
  return result;
}

template <int Rows, int Cols>
mat<Rows, Cols> operator*(const mat<Rows, Cols>& m, double t) {
  mat<Rows, Cols> result;
  for (int i = Rows; i--; result[i] = m[i] * t);
  return result;
}

template <int Rows, int Cols>
mat<Rows, Cols> operator*(double t, const mat<Rows, Cols>& m) {
  return m * t;
}

template <int Rows, int Cols>
mat<Rows, Cols> operator/(const mat<Rows, Cols>& m, double t) {
  mat<Rows, Cols> result;
  for (int i = Rows; i--; result[i] = m[i] / t);
  return result;
}

template <int Rows, int Cols>
mat<Rows, Cols> operator+(const mat<Rows, Cols>& a, const mat<Rows, Cols>& b) {
  mat<Rows, Cols> result;
  for (int i = Rows; i--;)
    for (int j = Cols; j--; result[i][j] = a[i][j] + b[i][j]);
  return result;
}

template <int Rows, int Cols>
mat<Rows, Cols> operator-(const mat<Rows, Cols>& a, const mat<Rows, Cols>& b) {
  mat<Rows, Cols> result;
  for (int i = Rows; i--;)
    for (int j = Cols; j--; result[i][j] = a[i][j] - b[i][j]);
  return result;
}

template <int Rows, int Cols>
std::ostream& operator<<(std::ostream& out, const mat<Rows, Cols>& m) {
  for (int i = 0; i < Rows; ++i) out << m[i] << "\n";
  return out;
}

inline vec3 operator*(const mat4& m, const vec3& v) {
  double x = m[0][0] * v.x() + m[0][1] * v.y() + m[0][2] * v.z() + m[0][3];
  double y = m[1][0] * v.x() + m[1][1] * v.y() + m[1][2] * v.z() + m[1][3];
  double z = m[2][0] * v.x() + m[2][1] * v.y() + m[2][2] * v.z() + m[2][3];
  double w = m[3][0] * v.x() + m[3][1] * v.y() + m[3][2] * v.z() + m[3][3];
  if (w != 0 && w != 1) return vec3(x / w, y / w, z / w);
  return vec3(x, y, z);

}

#endif  // RASTERIZER_MAT_H