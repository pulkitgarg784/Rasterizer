#ifndef RASTERIZER_MAT_H
#define RASTERIZER_MAT_H

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "vec.h"

template <int N>
struct mat {
  std::array<std::array<double, N>, N> data{};

  mat() {
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        data[i][j] = 0.0;
  }

  double& operator()(int row, int col) { return data[row][col]; }
  double operator()(int row, int col) const { return data[row][col]; }

  std::array<double, N>& operator[](int row) { return data[row]; }
  const std::array<double, N>& operator[](int row) const { return data[row]; }

  static mat<N> identity() {
    mat<N> m;
    for (int i = 0; i < N; ++i) m.data[i][i] = 1.0;
    return m;
  }

  mat<N> operator+(const mat<N>& other) const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        result.data[i][j] = data[i][j] + other.data[i][j];
    return result;
  }

  mat<N> operator-(const mat<N>& other) const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        result.data[i][j] = data[i][j] - other.data[i][j];
    return result;
  }

  mat<N> operator*(double t) const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        result.data[i][j] = data[i][j] * t;
    return result;
  }

  friend mat<N> operator*(double t, const mat<N>& m) { return m * t; }

  mat<N> operator*(const mat<N>& other) const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        for (int k = 0; k < N; ++k)
          result.data[i][j] += data[i][k] * other.data[k][j];
    return result;
  }

  mat<N> transpose() const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        result.data[i][j] = data[j][i];
    return result;
  }

  double determinant() const {
    if constexpr (N == 1) {
      return data[0][0];
    } else if constexpr (N == 2) {
      return data[0][0] * data[1][1] - data[0][1] * data[1][0];
    } else if constexpr (N == 3) {
      return data[0][0] * (data[1][1] * data[2][2] - data[1][2] * data[2][1]) -
             data[0][1] * (data[1][0] * data[2][2] - data[1][2] * data[2][0]) +
             data[0][2] * (data[1][0] * data[2][1] - data[1][1] * data[2][0]);
    } else {
      double det = 0;
      for (int j = 0; j < N; ++j) {
        det += (j % 2 == 0 ? 1 : -1) * data[0][j] * minor(0, j).determinant();
      }
      return det;
    }
  }

  mat<N - 1> minor(int row, int col) const {
    static_assert(N > 1, "Cannot compute minor of 1x1 matrix");
    mat<N - 1> result;
    int ri = 0;
    for (int i = 0; i < N; ++i) {
      if (i == row) continue;
      int rj = 0;
      for (int j = 0; j < N; ++j) {
        if (j == col) continue;
        result.data[ri][rj] = data[i][j];
        ++rj;
      }
      ++ri;
    }
    return result;
  }

  mat<N> cofactor() const {
    mat<N> result;
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j)
        result.data[i][j] = ((i + j) % 2 == 0 ? 1 : -1) * minor(i, j).determinant();
    return result;
  }

  mat<N> adjoint() const { return cofactor().transpose(); }

  mat<N> inverse() const {
    double det = determinant();
    if (std::abs(det) < 1e-10) {
      throw std::runtime_error("Matrix is singular and cannot be inverted");
    }
    return adjoint() * (1.0 / det);
  }

  friend std::ostream& operator<<(std::ostream& out, const mat<N>& m) {
    out << "[";
    for (int i = 0; i < N; ++i) {
      if (i > 0) out << " ";
      out << "[";
      for (int j = 0; j < N; ++j) {
        out << m.data[i][j];
        if (j < N - 1) out << ", ";
      }
      out << "]";
      if (i < N - 1) out << "\n";
    }
    out << "]";
    return out;
  }
};

template <>
inline mat<0> mat<1>::minor(int, int) const {
  return mat<0>();
}

using mat2 = mat<2>;
using mat3 = mat<3>;
using mat4 = mat<4>;

inline vec2 operator*(const mat2& m, const vec2& v) {
  return vec2(m[0][0] * v.x() + m[0][1] * v.y(),
              m[1][0] * v.x() + m[1][1] * v.y());
}

inline vec3 operator*(const mat3& m, const vec3& v) {
  return vec3(m[0][0] * v.x() + m[0][1] * v.y() + m[0][2] * v.z(),
              m[1][0] * v.x() + m[1][1] * v.y() + m[1][2] * v.z(),
              m[2][0] * v.x() + m[2][1] * v.y() + m[2][2] * v.z());
}

inline vec4 operator*(const mat4& m, const vec4& v) {
  return vec4(m[0][0] * v.x() + m[0][1] * v.y() + m[0][2] * v.z() + m[0][3] * v.w(),
              m[1][0] * v.x() + m[1][1] * v.y() + m[1][2] * v.z() + m[1][3] * v.w(),
              m[2][0] * v.x() + m[2][1] * v.y() + m[2][2] * v.z() + m[2][3] * v.w(),
              m[3][0] * v.x() + m[3][1] * v.y() + m[3][2] * v.z() + m[3][3] * v.w());
}

// Utility functions to generate transformation matrices
namespace transform {

inline mat4 translate(double x, double y, double z) {
  mat4 m = mat4::identity();
  m[0][3] = x;
  m[1][3] = y;
  m[2][3] = z;
  return m;
}

inline mat4 translate(const vec3& v) { return translate(v.x(), v.y(), v.z()); }

inline mat4 scale(double x, double y, double z) {
  mat4 m = mat4::identity();
  m[0][0] = x;
  m[1][1] = y;
  m[2][2] = z;
  return m;
}

inline mat4 scale(const vec3& v) { return scale(v.x(), v.y(), v.z()); }
inline mat4 scale(double s) { return scale(s, s, s); }

inline mat4 rotate_x(double angle) {
  mat4 m = mat4::identity();
  double c = std::cos(angle);
  double s = std::sin(angle);
  m[1][1] = c;  m[1][2] = -s;
  m[2][1] = s;  m[2][2] = c;
  return m;
}

inline mat4 rotate_y(double angle) {
  mat4 m = mat4::identity();
  double c = std::cos(angle);
  double s = std::sin(angle);
  m[0][0] = c;  m[0][2] = s;
  m[2][0] = -s; m[2][2] = c;
  return m;
}

inline mat4 rotate_z(double angle) {
  mat4 m = mat4::identity();
  double c = std::cos(angle);
  double s = std::sin(angle);
  m[0][0] = c;  m[0][1] = -s;
  m[1][0] = s;  m[1][1] = c;
  return m;
}

inline mat4 look_at(const vec3& eye, const vec3& center, const vec3& up) {
  vec3 f = normalize(center - eye);
  vec3 r = normalize(cross(f, up));
  vec3 u = cross(r, f);

  mat4 m = mat4::identity();
  m[0][0] = r.x();  m[0][1] = r.y();  m[0][2] = r.z();  m[0][3] = -dot(r, eye);
  m[1][0] = u.x();  m[1][1] = u.y();  m[1][2] = u.z();  m[1][3] = -dot(u, eye);
  m[2][0] = -f.x(); m[2][1] = -f.y(); m[2][2] = -f.z(); m[2][3] = dot(f, eye);
  return m;
}

inline mat4 perspective(double fov, double aspect, double near, double far) {
  double tan_half_fov = std::tan(fov / 2.0);
  mat4 m;
  m[0][0] = 1.0 / (aspect * tan_half_fov);
  m[1][1] = 1.0 / tan_half_fov;
  m[2][2] = -(far + near) / (far - near);
  m[2][3] = -(2.0 * far * near) / (far - near);
  m[3][2] = -1.0;
  return m;
}

inline mat4 orthographic(double left, double right, double bottom, double top,
                         double near, double far) {
  mat4 m = mat4::identity();
  m[0][0] = 2.0 / (right - left);
  m[1][1] = 2.0 / (top - bottom);
  m[2][2] = -2.0 / (far - near);
  m[0][3] = -(right + left) / (right - left);
  m[1][3] = -(top + bottom) / (top - bottom);
  m[2][3] = -(far + near) / (far - near);
  return m;
}

}  // namespace transform

#endif  // RASTERIZER_MAT_H