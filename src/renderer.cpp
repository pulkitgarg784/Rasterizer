#include "renderer.h"
#include <iostream>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

extern mat4 ModelView, Perspective, Viewport;
extern std::vector<float> zbuffer;

Mesh create_sphere_model(float radius, int rings, int sectors) {
    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    std::vector<int> faces;
    std::vector<int> face_normals;
    std::vector<int> face_uvs;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            float const y = std::sin(-M_PI_2 + M_PI * r * R);
            float const x = std::cos(2 * M_PI * s * S) * std::sin(M_PI * r * R);
            float const z = std::sin(2 * M_PI * s * S) * std::sin(M_PI * r * R);

            vertices.push_back(vec3{x * radius, y * radius, z * radius});
            normals.push_back(vec3{x, y, z});
            uvs.push_back(vec2{s * S, r * R});
        }
    }

    for (int r = 0; r < rings - 1; r++) {
        for (int s = 0; s < sectors - 1; s++) {
            int cur = r * sectors + s;
            int next = (r + 1) * sectors + s;
            
            faces.push_back(cur);
            faces.push_back(next);
            faces.push_back(cur + 1);
            
            face_normals.push_back(cur);
            face_normals.push_back(next);
            face_normals.push_back(cur + 1);

            face_uvs.push_back(cur);
            face_uvs.push_back(next);
            face_uvs.push_back(cur + 1);

            faces.push_back(cur + 1);
            faces.push_back(next);
            faces.push_back(next + 1);

            face_normals.push_back(cur + 1);
            face_normals.push_back(next);
            face_normals.push_back(next + 1);

            face_uvs.push_back(cur + 1);
            face_uvs.push_back(next);
            face_uvs.push_back(next + 1);
        }
    }
    
    return Mesh(vertices, faces, normals, face_normals, uvs, face_uvs);
}

struct PhongShader : IShader {
    const Mesh &mesh;
    vec3 l; // light position in View Space
    vec3 tri[3];
    vec3 nrmls[3];
    vec2 uv[3];
    mat<3,3> varying_tri;
    bool is_point;
    TGAColor color;

  PhongShader(const vec3 light, const Mesh& m, const mat4& View, bool point_light = false, TGAColor c = {255, 255, 255, 255}) 
      : mesh(m), is_point(point_light), color(c) {
    if (is_point) {
        vec4 light_transformed = View * vec4{light.x(), light.y(), light.z(), 1.};
        l = light_transformed.xyz();
    } else {
        vec4 light_transformed = View * vec4{light.x(), light.y(), light.z(), 0.};
        l = normalize(light_transformed.xyz());
    }
  }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = mesh.vertex(face, vert);
        vec3 n = mesh.normal(face, vert);
        uv[vert] = mesh.uv(face, vert);
        nrmls[vert] = (ModelView.invert_transpose() * vec4{n.x(), n.y(), n.z(), 0.}).xyz();
        vec4 gl_Position = ModelView * vec4{v.x(), v.y(), v.z(), 1.};
        tri[vert] = gl_Position.xyz();
        varying_tri.rows[vert] = tri[vert];
        return Perspective * gl_Position;
    }

    virtual std::pair<bool,TGAColor> fragment(const vec3 bar) const {
        TGAColor gl_FragColor = color;
        vec2 uv_interp = uv[0]*bar[0] + uv[1]*bar[1] + uv[2]*bar[2];
        TGAColor tex_color = mesh.diffuse(uv_interp);
        for (int i=0; i<3; i++) gl_FragColor[i] = (gl_FragColor[i] * tex_color[i]) / 255;
        
        vec3 n = normalize(nrmls[0]*bar[0] + nrmls[1]*bar[1] + nrmls[2]*bar[2]); // normal vector (smooth shading)
        
        if (mesh.hasNormalMap()) {
            mat<3,3> A;
            A[0] = varying_tri.rows[1] - varying_tri.rows[0];
            A[1] = varying_tri.rows[2] - varying_tri.rows[0];
            A[2] = n;
            mat<3,3> AI = A.invert();
            vec3 i = AI * vec3(uv[1][0] - uv[0][0], uv[2][0] - uv[0][0], 0);
            vec3 j = AI * vec3(uv[1][1] - uv[0][1], uv[2][1] - uv[0][1], 0);
            mat<3,3> B;
            B[0] = i.xyz();
            B[1] = j.xyz();
            B[2] = n;
            vec3 texture_n = mesh.normal(uv_interp);
            n = normalize(B.transpose() * texture_n);
        }

        vec3 light_dir_vec;
        if (is_point) {
            vec3 p = tri[0]*bar[0] + tri[1]*bar[1] + tri[2]*bar[2]; // fragment position in View Space
            light_dir_vec = normalize(l - p);
        } else {
            light_dir_vec = l;
        }

        vec3 r = normalize(n * (dot(n, light_dir_vec) * 2.0) - light_dir_vec); // relflection vector
        double ambient = .3;
        double diff = std::max(0., dot(n, light_dir_vec));
        double spec = std::pow(std::max(r.z(), 0.), 35);
        for (int channel : {0,1,2})
            gl_FragColor[channel] *= std::min(1., ambient + .4*diff + .9*spec);
        return {false, gl_FragColor};
    }
};

// --- Renderer Implementation ---

Renderer::Renderer(int w, int h) 
    : width(w), height(h), framebuffer(w, h, TGAImage::RGB),
      eye({-1, 0, 2}), center({0, 0, 0}), up({0, 1, 0}), light_dir({1, 1, 1}) {}

Renderer::~Renderer() {
    for (auto obj : objects) delete obj;
    
    shutdown_imgui();
    
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Renderer::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "Could not init SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Renderer", 100, 100, width, height, SDL_WINDOW_SHOWN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, width, height);
    
    init_imgui();
    
    // Init graphics pipeline
    lookat(eye, center, up);
    init_perspective(norm(eye - center));
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_zbuffer(width, height);
    
    last_time = SDL_GetTicks();
    
    return true;
}

void Renderer::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
}

void Renderer::shutdown_imgui() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool Renderer::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            return false;
        }
    }
    
    // Update time
    Uint32 current_time = SDL_GetTicks();
    dt = (current_time - last_time) / 1000.0f;
    last_time = current_time;
    
    return true;
}

RenderObject* Renderer::create_sphere(float radius, TGAColor color, int rings, int sectors) {
    Mesh m = create_sphere_model(radius, rings, sectors);
    RenderObject* obj = new RenderObject(m, {0,0,0}, color);
    objects.push_back(obj);
    return obj;
}

RenderObject* Renderer::load_mesh(const std::string& filename, TGAColor color) {
    Mesh m(filename);
    m.normalize();
    RenderObject* obj = new RenderObject(m, {0,0,0}, color);
    objects.push_back(obj);
    return obj;
}

void Renderer::set_camera(vec3 e, vec3 c, vec3 u) {
    eye = e; center = c; up = u;
    lookat(eye, center, up);
    init_perspective(norm(eye - center));
}

void Renderer::set_light_dir(vec3 dir) {
    light_dir = dir;
}

void Renderer::add_ui_callback(std::function<void()> callback) {
    ui_callbacks.push_back(callback);
}

void Renderer::render() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // UI Callback
    for (auto& cb : ui_callbacks) cb();
    
    ImGui::Render();
    
    // Clear buffers
    framebuffer.clear();
    init_zbuffer(width, height);
    
    // Update view matrix
    lookat(eye, center, up);
    mat4 View = ModelView;
    
    // Render loop
    for (auto* obj : objects) {
        mat4 Translation = {{{1, 0, 0, obj->position[0]},
                             {0, 1, 0, obj->position[1]},
                             {0, 0, 1, obj->position[2]},
                             {0, 0, 0, 1}}};
        
        ModelView = View * Translation;
        
        #pragma omp parallel for
        for (int i = 0; i < obj->mesh.nfaces(); i++) {
            PhongShader shader(light_dir, obj->mesh, View, true, obj->color);
            Triangle clip = {shader.vertex(i, 0),
                             shader.vertex(i, 1),
                             shader.vertex(i, 2)};
            rasterize(clip, shader, framebuffer);
        }
    }
    
    ModelView = View; // Restore View matrix for next frame
    
    // Push framebuffer to SDL
    SDL_UpdateTexture(texture, nullptr, framebuffer.buffer(), width * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}
