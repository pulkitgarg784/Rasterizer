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

std::tuple<int, int> project(vec3 v) {
  return {(v.x + 1.) * width / 2, (v.y + 1.) * height / 2};
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

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
    return 1;
  }
  Model model(argv[1]);
  model.normalize();

  TGAImage framebuffer(width, height, TGAImage::RGB);

  for (int i = 0; i < model.nfaces(); ++i) {
    auto [ax, ay] = project(model.vertex(i, 0));
    auto [bx, by] = project(model.vertex(i, 1));
    auto [cx, cy] = project(model.vertex(i, 2));
    line(ax, ay, bx, by, framebuffer, white);
    line(bx, by, cx, cy, framebuffer, white);
    line(cx, cy, ax, ay, framebuffer, white);
  }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
