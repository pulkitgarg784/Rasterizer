#include "graphics.h"

#include <cstdlib>
#include <limits>
#include <algorithm>
#include <vector>
#include <mutex>
#include <memory>

#include "matrix.h"

mat<4,4> ModelView, Viewport, Perspective;
std::vector<float> zbuffer;
std::vector<std::unique_ptr<std::mutex>> tile_mutexes;
const int TILE_SIZE = 64;
int n_tiles_w = 0;
int n_tiles_h = 0;

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
      std::vector<float>(width * height, -std::numeric_limits<float>::max());

  int new_n_tiles_w = (width + TILE_SIZE - 1) / TILE_SIZE;
  int new_n_tiles_h = (height + TILE_SIZE - 1) / TILE_SIZE;

  if (new_n_tiles_w * new_n_tiles_h != (int)tile_mutexes.size()) {
    n_tiles_w = new_n_tiles_w;
    n_tiles_h = new_n_tiles_h;
    tile_mutexes.clear();
    tile_mutexes.reserve(n_tiles_w * n_tiles_h);
    for (int i = 0; i < n_tiles_w * n_tiles_h; ++i) {
      tile_mutexes.push_back(std::make_unique<std::mutex>());
    }
  }
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
  mat<3, 3> ABC_inv_t = ABC.invert_transpose();
  auto x_bounds = std::minmax({screen[0].x(), screen[1].x(), screen[2].x()});
  auto y_bounds = std::minmax({screen[0].y(), screen[1].y(), screen[2].y()});
  auto bbminx = x_bounds.first;
  auto bbmaxx = x_bounds.second;
  auto bbminy = y_bounds.first;
  auto bbmaxy = y_bounds.second;

  int min_tile_x = std::max(0, (int)bbminx / TILE_SIZE);
  int max_tile_x = std::min(n_tiles_w - 1, (int)bbmaxx / TILE_SIZE);
  int min_tile_y = std::max(0, (int)bbminy / TILE_SIZE);
  int max_tile_y = std::min(n_tiles_h - 1, (int)bbmaxy / TILE_SIZE);

  for (int ty = min_tile_y; ty <= max_tile_y; ty++) {
    for (int tx = min_tile_x; tx <= max_tile_x; tx++) {
      std::lock_guard<std::mutex> lock(*tile_mutexes[ty * n_tiles_w + tx]);

      int x_start = std::max((int)bbminx, tx * TILE_SIZE);
      int x_end = std::min({(int)bbmaxx, (tx + 1) * TILE_SIZE - 1, framebuffer.width() - 1});
      int y_start = std::max((int)bbminy, ty * TILE_SIZE);
      int y_end = std::min({(int)bbmaxy, (ty + 1) * TILE_SIZE - 1, framebuffer.height() - 1});

      for (int y = y_start; y <= y_end; y++) {
        for (int x = x_start; x <= x_end; x++) {
          vec3 bc = ABC_inv_t *
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
  }
}