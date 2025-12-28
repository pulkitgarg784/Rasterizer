#include "renderer.h"
#include "imgui.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

std::vector<std::string> get_files(const std::string& path, const std::string& ext) {
    std::vector<std::string> files;
    if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == ext) {
                files.push_back(entry.path().filename().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

struct PhysicsObject {
    RenderObject* render_obj;
    vec3 velocity;
    float radius;
    
    PhysicsObject(RenderObject* obj, vec3 vel, float r) 
        : render_obj(obj), velocity(vel), radius(r) {}
};

int main(int argc, char** argv) {
    std::srand(std::time(nullptr));
    
    Renderer renderer(800, 800);
    if (!renderer.init()) return 1;

    std::vector<PhysicsObject> physics_objects;
    
    // Load any obj files passed as command line arguments
    for (int i = 1; i < argc; i++) {
        renderer.load_mesh(argv[i]);
    }

    auto mesh_files = get_files("assets", ".obj");
    auto texture_files = get_files("assets", ".tga");
    
    // Helper to find index
    auto find_index = [](const std::vector<std::string>& files, const std::string& name) {
        auto it = std::find(files.begin(), files.end(), name);
        return (it != files.end()) ? std::distance(files.begin(), it) : 0;
    };

    int current_mesh_idx = find_index(mesh_files, "head.obj");
    int current_diffuse_idx = find_index(texture_files, "african_head_diffuse.tga");
    int current_nm_idx = find_index(texture_files, "african_head_nm_tangent.tga");

    RenderObject* obj = renderer.load_mesh("assets/head.obj");
    obj->mesh.load_texture("assets/african_head_diffuse.tga");
    obj->mesh.load_normal_map("assets/african_head_nm_tangent.tga");

    // UI Callback
    renderer.add_ui_callback([&]() {
        ImGui::Begin("Physics Engine");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::Checkbox("Enable Physics", &renderer.physics_enabled);

        ImGui::Separator();
        ImGui::Text("Mesh Selection");
        
        if (!mesh_files.empty()) {
            if (ImGui::BeginCombo("Mesh", mesh_files[current_mesh_idx].c_str())) {
                for (int n = 0; n < mesh_files.size(); n++) {
                    bool is_selected = (current_mesh_idx == n);
                    if (ImGui::Selectable(mesh_files[n].c_str(), is_selected)) {
                        current_mesh_idx = n;
                        obj->mesh = Mesh("assets/" + mesh_files[current_mesh_idx]);
                        // Re-apply textures
                        if (!texture_files.empty()) {
                             obj->mesh.load_texture("assets/" + texture_files[current_diffuse_idx]);
                             obj->mesh.load_normal_map("assets/" + texture_files[current_nm_idx]);
                        }
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        if (!texture_files.empty()) {
            if (ImGui::BeginCombo("Diffuse Texture", texture_files[current_diffuse_idx].c_str())) {
                for (int n = 0; n < texture_files.size(); n++) {
                    bool is_selected = (current_diffuse_idx == n);
                    if (ImGui::Selectable(texture_files[n].c_str(), is_selected)) {
                        current_diffuse_idx = n;
                        obj->mesh.load_texture("assets/" + texture_files[current_diffuse_idx]);
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Normal Map", texture_files[current_nm_idx].c_str())) {
                for (int n = 0; n < texture_files.size(); n++) {
                    bool is_selected = (current_nm_idx == n);
                    if (ImGui::Selectable(texture_files[n].c_str(), is_selected)) {
                        current_nm_idx = n;
                        obj->mesh.load_normal_map("assets/" + texture_files[current_nm_idx]);
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        
        if (ImGui::Button("Add Sphere")) {
            TGAColor color = {static_cast<uint8_t>(std::rand() % 256), 
                              static_cast<uint8_t>(std::rand() % 256), 
                              static_cast<uint8_t>(std::rand() % 256), 255};
            
            float radius = 0.2f;
            vec3 pos = {(std::rand() / (float)RAND_MAX) * 2.0f - 1.0f,
                        (std::rand() / (float)RAND_MAX) * 2.0f + 2.0f,
                        (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f};
            
            RenderObject* ro = renderer.create_sphere(radius, color);
            ro->position = pos;
            
            vec3 vel = {(std::rand() / (float)RAND_MAX) * 2.0f - 1.0f,
                        0.0f,
                        (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f};
            
            physics_objects.emplace_back(ro, vel, radius);
        }
        
        ImGui::End();
    });

    while (renderer.process_events()) {
        float dt = renderer.get_delta_time();
        
        if (renderer.physics_enabled) {
            for (auto& obj : physics_objects) {
                // Gravity
                obj.velocity[1] -= 9.8f * dt;
                
                // Update position
                obj.render_obj->position[0] += obj.velocity[0] * dt;
                obj.render_obj->position[1] += obj.velocity[1] * dt;
                obj.render_obj->position[2] += obj.velocity[2] * dt;
                
                // Floor collision
                if (obj.render_obj->position[1] < -1.0f + obj.radius) {
                    obj.render_obj->position[1] = -1.0f + obj.radius;
                    obj.velocity[1] *= -0.8f; // Damping
                }
            }
        }
        
        renderer.render();
    }

    return 0;
}
