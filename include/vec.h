#ifndef RASTERIZER_VEC_H
#define RASTERIZER_VEC_H

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

template <int N>
struct vec {
  std::array<double, N> data{};

  vec() { data.fill(0.0); }

  template <typename... Args>
  vec(Args... args) : data{static_cast<double>(args)...} {
    static_assert(sizeof...(args) == N, "Wrong number of arguments");
  }

  double& operator[](int i) { return data[i]; }
  double operator[](int i) const { return data[i]; }
  
  double x() const {
    static_assert(N >= 1, "x() is only available for N >= 1");
    return data[0];
  }
  double y() const {
    static_assert(N >= 2, "y() is only available for N >= 2");
    return data[1];
  }
  double z() const {
    static_assert(N >= 3, "z() is only available for N >= 3");
    return data[2];
  }
  double w() const {
    static_assert(N >= 4, "w() is only available for N >= 4");
    return data[3];
  }

  vec<N> operator+(const vec<N>& v) const {
    vec<N> result;
    for (int i = 0; i < N; ++i) result[i] = data[i] + v[i];
    return result;
  }

  vec<N> operator-(const vec<N>& v) const {
    vec<N> result;
    for (int i = 0; i < N; ++i) result[i] = data[i] - v[i];
    return result;
  }

  vec<N> operator*(double t) const {
    vec<N> result;
    for (int i = 0; i < N; ++i) result[i] = data[i] * t;
    return result;
  }

  vec<N> operator/(double t) const {
    vec<N> result;
    for (int i = 0; i < N; ++i) result[i] = data[i] / t;
    return result;
  }

  vec<N> operator-() const {
    vec<N> result;
    for (int i = 0; i < N; ++i) result[i] = -data[i];
    return result;
  }

  vec<N>& operator+=(const vec<N>& v) {
    for (int i = 0; i < N; ++i) data[i] += v[i];
    return *this;
  }

  vec<N>& operator-=(const vec<N>& v) {
    for (int i = 0; i < N; ++i) data[i] -= v[i];
    return *this;
  }

  vec<N>& operator*=(double t) {
    for (int i = 0; i < N; ++i) data[i] *= t;
    return *this;
  }

  vec<N>& operator/=(double t) {
    for (int i = 0; i < N; ++i) data[i] /= t;
    return *this;
  }

  double dot(const vec<N>& v) const {
    double result = 0;
    for (int i = 0; i < N; ++i) result += data[i] * v[i];
    return result;
  }

  double sqr_magnitude() const { return dot(*this); }
  double length() const { return std::sqrt(sqr_magnitude()); }

  vec<N> normalized() const {
    double l = length();
    return l > 0 ? *this / l : vec<N>();
  }


  friend vec<N> operator*(double t, const vec<N>& v) { return v * t; }

  friend std::ostream& operator<<(std::ostream& out, const vec<N>& v) {
    out << "(";
    for (int i = 0; i < N; ++i) {
      out << v[i];
      if (i < N - 1) out << ", ";
    }
    out << ")";
    return out;
  }
};

using vec2 = vec<2>;
using vec3 = vec<3>;
using vec4 = vec<4>;

template <int N>
double dot(const vec<N>& u, const vec<N>& v) { return u.dot(v); }

template <int N>
vec<N> normalize(const vec<N>& v) { return v.normalized(); }

// Cross product for 3D vectors
inline vec3 cross(const vec3& u, const vec3& v) {
  return vec3(u[1] * v[2] - u[2] * v[1],
                u[2] * v[0] - u[0] * v[2],
                u[0] * v[1] - u[1] * v[0]);
}

template <int N>
double norm(const vec<N>& v) { return std::sqrt(v.dot(v)); }

template <int N> double& x(vec<N>& v) { static_assert(N >= 1); return v[0]; }
template <int N> double& y(vec<N>& v) { static_assert(N >= 2); return v[1]; }
template <int N> double& z(vec<N>& v) { static_assert(N >= 3); return v[2]; }
template <int N> double& w(vec<N>& v) { static_assert(N >= 4); return v[3]; }

template <int N> double x(const vec<N>& v) { static_assert(N >= 1); return v[0]; }
template <int N> double y(const vec<N>& v) { static_assert(N >= 2); return v[1]; }
template <int N> double z(const vec<N>& v) { static_assert(N >= 3); return v[2]; }
template <int N> double w(const vec<N>& v) { static_assert(N >= 4); return v[3]; }

#endif  // RASTERIZER_VEC_H