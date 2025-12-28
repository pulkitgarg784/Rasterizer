#ifndef RASTERIZER_RENDERER_H
#define RASTERIZER_RENDERER_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "graphics.h"
#include "mesh.h"
#include "tgaimage.h"
#include "matrix.h"

struct RenderObject {
    vec3 position;
    Mesh mesh;
    TGAColor color;
    
    RenderObject(Mesh m, vec3 pos, TGAColor c) : mesh(m), position(pos), color(c) {}
};

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    bool init();
    bool process_events(); // returns false on quit
    void render();
    
    // Mesh creation
    RenderObject* create_sphere(float radius, TGAColor color, int rings = 20, int sectors = 20);
    RenderObject* load_mesh(const std::string& filename, TGAColor color = {255, 255, 255, 255});

    // Time utils
    float get_delta_time() const { return dt; }
    Uint32 get_ticks() const { return SDL_GetTicks(); }

    // Camera
    void set_camera(vec3 eye, vec3 center, vec3 up);
    void set_light_dir(vec3 dir);

    // Lighting
    vec3 light_dir;
    float light_intensity = 1.0f;

    // Debug UI
    bool physics_enabled = true;
    void add_ui_callback(std::function<void()> callback);

private:
    int width, height;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    
    TGAImage framebuffer;
    
    std::vector<RenderObject*> objects;
    
    vec3 eye, center, up;
    
    Uint32 last_time = 0;
    float dt = 0.0f;

    std::vector<std::function<void()>> ui_callbacks;

    void init_imgui();
    void shutdown_imgui();
};

#endif // RASTERIZER_RENDERER_H
