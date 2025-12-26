#include <SDL2/SDL.h>

#include <cstdio>
#include <iostream>

#include "matrix.h"
#include "model.h"
#include "tgaimage.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_stdlib.h"

constexpr TGAColor white = {255, 255, 255, 255};
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

constexpr int width = 800;
constexpr int height = 800;

mat4 ModelView, Viewport, Perspective;

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

void perspective(const double f) {
  // Scale x and y by 1/f to maintain constant FOV (90 degrees)
  // otherwise the object size stays constant on screen (Dolly Zoom effect)
  double d = (f < 1e-6) ? 1e-6 : f;
  Perspective = {{{1/d, 0, 0, 0}, {0, 1/d, 0, 0}, {0, 0, 1, 0}, {0, 0, -1 / d, 1}}};
}

void viewport(const int x, const int y, const int w, const int h) {
  Viewport = {{{w / 2., 0, 0, x + w / 2.},
               {0, h / 2., 0, y + h / 2.},
               {0, 0, 1, 0},
               {0, 0, 0, 1}}};
}

void rasterize(const vec4 clip[3], std::vector<double>& zbuffer,
               TGAImage& framebuffer, const TGAColor color) {
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
  auto [bbminx, bbmaxx] =
      std::minmax({screen[0].x(), screen[1].x(), screen[2].x()});
  auto [bbminy, bbmaxy] =
      std::minmax({screen[0].y(), screen[1].y(), screen[2].y()});
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
      zbuffer[x + y * framebuffer.width()] = z;
      framebuffer.set(x, framebuffer.height() - 1 - y, color);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Could not init SDL: " << SDL_GetError() << std::endl;
    return 1;
  }

  constexpr int width = 800;
  constexpr int height = 800;

  SDL_Window* window =
      SDL_CreateWindow("Renderer", 100, 100, width, height, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "Could not create SDL window: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    SDL_DestroyWindow(window);
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  SDL_Texture* texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24,
                        SDL_TEXTUREACCESS_STREAMING, width, height);

  vec3 eye{-1, 0, 2};
  vec3 center{0, 0, 0};
  vec3 up{0, 1, 0};

  lookat(eye, center, up);
  perspective(norm(eye - center));
  viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);

  TGAImage framebuffer(width, height, TGAImage::RGB);
  std::vector<double> zbuffer(width * height,
                              -std::numeric_limits<double>::max());

  bool end = false;
  SDL_Event event;
  
  Uint32 frameCount = 0;
  Uint32 lastTime = SDL_GetTicks();

  while (!end) {
    // Handle Events
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && 
          event.key.keysym.sym == SDLK_ESCAPE) {
        end = true;
      }
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Info");
    ImGui::Text("FPS: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    static bool lock_target = false;
    ImGui::Checkbox("Lock Target (Strafe)", &lock_target);

    bool camera_changed = false;
    float eye_pos[3] = { (float)eye[0], (float)eye[1], (float)eye[2] };
    if (ImGui::DragFloat3("Camera Position", eye_pos, 0.1f)) {
        if (lock_target) {
            center[0] += eye_pos[0] - eye[0];
            center[1] += eye_pos[1] - eye[1];
            center[2] += eye_pos[2] - eye[2];
        }
        eye[0] = eye_pos[0];
        eye[1] = eye_pos[1];
        eye[2] = eye_pos[2];
        camera_changed = true;
    }
    
    float center_pos[3] = { (float)center[0], (float)center[1], (float)center[2] };
    if (ImGui::DragFloat3("Camera Target", center_pos, 0.1f)) {
        center[0] = center_pos[0];
        center[1] = center_pos[1];
        center[2] = center_pos[2];
        camera_changed = true;
    }

    if (camera_changed) {
        lookat(eye, center, up);
        perspective(norm(eye - center));
    }

    ImGui::End();

    // Rendering
    ImGui::Render();

    // Clear Buffers
    framebuffer.clear();
    std::fill(zbuffer.begin(), zbuffer.end(),
              -std::numeric_limits<double>::max());

    // Rendering loop
    for (int m = 1; m < argc; m++) {
      Model model(argv[m]);
      for (int i = 0; i < model.nfaces(); i++) {
        vec4 clip[3];
        for (int d : {0, 1, 2}) {
          vec3 v = model.vertex(i, d);
          clip[d] = Perspective * ModelView * vec4{v.x(), v.y(), v.z(), 1.};
        }
        TGAColor color = white;
        for (int c = 0; c < 3; c++) color[c] = std::rand() % 255;

        rasterize(clip, zbuffer, framebuffer, color);
      }
    }

    // Update Screen
    SDL_UpdateTexture(texture, nullptr, framebuffer.buffer(), width * 3);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
  
  }

  // Cleanup
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}