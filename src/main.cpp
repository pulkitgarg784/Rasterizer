#include <iostream>

#include "model.h"
#include "tgaimage.h"

constexpr TGAColor white = {255, 255, 255, 255};  // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

constexpr int width = 800;
constexpr int height = 800;

std::tuple<int, int, int> project(vec3 v) {
  return {(v.x + 1.) * width / 2, (v.y + 1.) * height / 2,
          (v.z + 1.) * 255.0 / 2};
}

void line(int ax, int ay, int bx, int by, TGAImage& framebuffer,
          TGAColor color) {
  bool steep = std::abs(ax - bx) < std::abs(ay - by);
  if (steep) {  // if the line is steep, we transpose the image
    std::swap(ax, ay);
    std::swap(bx, by);
  }
  if (ax > bx) {  // make it left−to−right
    std::swap(ax, bx);
    std::swap(ay, by);
  }
  for (int x = ax; x <= bx; x++) {
    float t = (x - ax) / static_cast<float>(bx - ax);
    int y = std::round(ay + (by - ay) * t);
    if (steep)  // if transposed, de−transpose
      framebuffer.set(y, x, color);
    else
      framebuffer.set(x, y, color);
  }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
}

void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy,
              int cz, TGAImage& zbuffer, TGAImage& framebuffer,
              TGAColor color) {
  int bbminx = std::min(std::min(ax, bx), cx);
  int bbminy = std::min(std::min(ay, by), cy);
  int bbmaxx = std::max(std::max(ax, bx), cx);
  int bbmaxy = std::max(std::max(ay, by), cy);
  double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
  if (total_area < 1) return;
#pragma omp parallel for
  for (int x = bbminx; x <= bbmaxx; x++) {
    for (int y = bbminy; y <= bbmaxy; y++) {
      double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
      double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
      double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
      if (alpha < 0 || beta < 0 || gamma < 0) continue;
      unsigned char z =
          static_cast<unsigned char>(alpha * az + beta * bz + gamma * cz);
      if (z <= zbuffer.get(x, y)[0]) continue;
      zbuffer.set(x, y, {z});
      framebuffer.set(x, y, color);
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
    return 1;
  }
  Model model(argv[1]);
  model.normalize();

  TGAImage framebuffer(width, height, TGAImage::RGB);
  TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
  for (int i = 0; i < model.nfaces(); ++i) {
    auto [ax, ay, az] = project(model.vertex(i, 0));
    auto [bx, by, bz] = project(model.vertex(i, 1));
    auto [cx, cy, cz] = project(model.vertex(i, 2));
    TGAColor rnd;
    for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
    triangle(ax, ay, az, bx, by, bz, cx, cy, cz, zbuffer, framebuffer, rnd);
  }

  framebuffer.write_tga_file("framebuffer.tga");
  zbuffer.write_tga_file("zbuffer.tga");
  return 0;
}
