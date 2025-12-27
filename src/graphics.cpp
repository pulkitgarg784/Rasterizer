#include "graphics.h"

#include <cstdlib>

#include "matrix.h"

mat<4,4> ModelView, Viewport, Perspective;
std::vector<double> zbuffer;

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
  vec3 n = normalize(eye - center);
  vec3 l = normalize(cross(up, n));
  vec3 m = normalize(cross(n, l));
  ModelView = mat<4, 4>{{{l.x(), l.y(), l.z(), 0},
                         {m.x(), m.y(), m.z(), 0},
                         {n.x(), n.y(), n.z(), 0},
                         {0, 0, 0, 1}}} *
              mat<4, 4>{{{1, 0, 0, -center.x()},
                         {0, 1, 0, -center.y()},
                         {0, 0, 1, -center.z()},
                         {0, 0, 0, 1}}};
}

void init_perspective(const double f) {
  double d = (f < 1e-6) ? 1e-6 : f;
  Perspective = {
      {{1 / d, 0, 0, 0}, {0, 1 / d, 0, 0}, {0, 0, 1, 0}, {0, 0, -1 / d, 1}}};
}

void init_viewport(const int x, const int y, const int w, const int h) {
  Viewport = {{{w / 2., 0, 0, x + w / 2.},
               {0, h / 2., 0, y + h / 2.},
               {0, 0, 1, 0},
               {0, 0, 0, 1}}};
}

void init_zbuffer(const int width, const int height) {
  zbuffer =
      std::vector<double>(width * height, -std::numeric_limits<double>::max());
}

void rasterize(const Triangle& clip, const IShader& shader,
               TGAImage& framebuffer) {
  vec4 ndc[3] = {clip[0] / clip[0].w(), clip[1] / clip[1].w(),
                 clip[2] / clip[2].w()};
  vec4 s0 = Viewport * ndc[0];
  vec4 s1 = Viewport * ndc[1];
  vec4 s2 = Viewport * ndc[2];
  vec2 screen[3] = {vec2(s0.x(), s0.y()), vec2(s1.x(), s1.y()),
                    vec2(s2.x(), s2.y())};

  mat<3, 3> ABC = {{{screen[0].x(), screen[0].y(), 1.},
                    {screen[1].x(), screen[1].y(), 1.},
                    {screen[2].x(), screen[2].y(), 1.}}};
  if (ABC.det() < 1) return;
  auto x_bounds = std::minmax({screen[0].x(), screen[1].x(), screen[2].x()});
  auto y_bounds = std::minmax({screen[0].y(), screen[1].y(), screen[2].y()});
  auto bbminx = x_bounds.first;
  auto bbmaxx = x_bounds.second;
  auto bbminy = y_bounds.first;
  auto bbmaxy = y_bounds.second;
#pragma omp parallel for
  for (int x = std::max<int>(bbminx, 0);
       x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) {
    for (int y = std::max<int>(bbminy, 0);
         y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++) {
      vec3 bc = ABC.invert_transpose() *
                vec3{static_cast<double>(x), static_cast<double>(y), 1.};
      if (bc.x() < 0 || bc.y() < 0 || bc.z() < 0) continue;
      double z = dot(bc, vec3(ndc[0].z(), ndc[1].z(), ndc[2].z()));
      if (z <= zbuffer[x + y * framebuffer.width()]) continue;
      auto [discard, color] = shader.fragment(bc);
      if (discard) continue;
      zbuffer[x + y * framebuffer.width()] = z;
      framebuffer.set(x, framebuffer.height() - 1 - y, color);
    }
  }
}