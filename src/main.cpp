#include <SDL2/SDL.h>

#include <cstdio>
#include <iostream>

#include "graphics.h"
#include "matrix.h"

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

extern mat4 ModelView, Perspective;
extern std::vector<double> zbuffer;

struct RandomShader : IShader {
  const Model &model;
  const mat4 &modelViewMatrix;
  TGAColor color = {};
  vec3 tri[3];

  RandomShader(const Model &m, const mat4 &mv) : model(m), modelViewMatrix(mv) {}

  virtual vec4 vertex(const int iface, const int nthvert) {
    vec3 v = model.vertex(iface, nthvert);
    vec4 v_pos = modelViewMatrix * vec4{v.x(), v.y(), v.z(), 1.};
    tri[nthvert] = vec3{v_pos.x(), v_pos.y(), v_pos.z()};
    return Perspective * v_pos;
  }

  virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
    TGAColor color;
    for (int c = 0; c < 3; c++) color[c] = std::rand() % 255;
    return {false, color};
  }
};

struct PhongShader : IShader {
    const Model &model;
    vec3 l; // light direction
    vec3 tri[3];

    PhongShader(const vec3 light, const Model &m) : model(m) {
        vec4 light_transformed = ModelView*vec4{light.x(), light.y(), light.z(), 0.};
        l = normalize(vec3{light_transformed.x(), light_transformed.y(), light_transformed.z()});
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vertex(face, vert);                          // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{v.x(), v.y(), v.z(), 1.};
        tri[vert] = vec3{gl_Position.x(), gl_Position.y(), gl_Position.z()};                            // in eye coordinates
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        TGAColor gl_FragColor = {255, 255, 255, 255};             // output color of the fragment
        vec3 n = normalize(cross(tri[1]-tri[0], tri[2]-tri[0])); // triangle normal in eye coordinates
        vec3 r = normalize(n * (dot(n, l) * 2.0) - l);           // reflected light direction
        double ambient = .3;                                      // ambient light intensity
        double diff = std::max(0., dot(n, l));                    // diffuse light intensity
        double spec = std::pow(std::max(r.z(), 0.), 35);          // specular intensity, note that the camera lies on the z-axis (in eye coordinates), therefore simple r.z, since (0,0,1)*(r.x, r.y, r.z) = r.z
        for (int channel : {0,1,2})
            gl_FragColor[channel] *= std::min(1., ambient + .4*diff + .9*spec);
        return {false, gl_FragColor};                             // do not discard the pixel
    }
};


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
  vec3 light_dir{1, 1, 1};

  // Model rotation angles (in degrees)
  float rotation[3] = {0.0f, 0.0f, 0.0f};

  lookat(eye, center, up);
  init_perspective(norm(eye - center));
  init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
  init_zbuffer(width, height);

  TGAImage framebuffer(width, height, TGAImage::RGB);

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
    ImGui::Text("FPS: %.1f FPS (%.3f ms/frame)", io.Framerate, 1000.0f / io.Framerate);

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
        init_perspective(norm(eye - center));
    }

    // Model rotation sliders
    ImGui::Separator();
        float rotation_angle[3] = { (float)rotation[0], (float)rotation[1], (float)rotation[2] };
        if (ImGui::DragFloat3("Rotation Angle", rotation_angle, 1.0f)) {
        rotation[0] = rotation_angle[0];
        rotation[1] = rotation_angle[1];
        rotation[2] = rotation_angle[2];
    }
    // ImGui::SliderFloat("Rotate X", &rotation[0], -180.0f, 180.0f, "%.1f deg");
    // ImGui::SliderFloat("Rotate Y", &rotation[1], -180.0f, 180.0f, "%.1f deg");
    // ImGui::SliderFloat("Rotate Z", &rotation[2], -180.0f, 180.0f, "%.1f deg");
    if (ImGui::Button("Reset Rotation")) {
        rotation[0] = rotation[1] = rotation[2] = 0.0f;
    }

    ImGui::Separator();
    float light_pos[3] = { (float)light_dir[0], (float)light_dir[1], (float)light_dir[2] };
    if (ImGui::DragFloat3("Light Direction", light_pos, 0.1f)) {
        light_dir[0] = light_pos[0];
        light_dir[1] = light_pos[1];
        light_dir[2] = light_pos[2];
    }

    ImGui::End();

    // Rendering
    ImGui::Render();

    // Clear Buffers
    framebuffer.clear();
    init_zbuffer(width, height);
    lookat(eye, center, up);

    // Build rotation matrix from Euler angles (X, Y, Z order)
    double rx = rotation[0] * M_PI / 180.0;
    double ry = rotation[1] * M_PI / 180.0;
    double rz = rotation[2] * M_PI / 180.0;
    
    double cx = cos(rx), sx = sin(rx);
    double cy = cos(ry), sy = sin(ry);
    double cz = cos(rz), sz = sin(rz);
    
    mat4 rotX = {{{1, 0, 0, 0}, {0, cx, -sx, 0}, {0, sx, cx, 0}, {0, 0, 0, 1}}};
    mat4 rotY = {{{cy, 0, sy, 0}, {0, 1, 0, 0}, {-sy, 0, cy, 0}, {0, 0, 0, 1}}};
    mat4 rotZ = {{{cz, -sz, 0, 0}, {sz, cz, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
    
    mat4 rotationMatrix = rotZ * rotY * rotX;
    ModelView = ModelView * rotationMatrix;

    // Rendering loop
    for (int m = 1; m < argc; m++) {
      Model model(argv[m]);
      // RandomShader shader(model, fullModelView);
      PhongShader shader(light_dir, model);
      for (int i = 0; i < model.nfaces(); i++) {
        Triangle clip = {shader.vertex(i, 0),
                         shader.vertex(i, 1),
                         shader.vertex(i, 2)};
        rasterize(clip, shader, framebuffer);
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