#include "renderer.h"
#include "imgui.h"
#include <vector>
#include <cstdlib>
#include <ctime>

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

    RenderObject* obj = renderer.load_mesh("assets/head.obj");
    obj->mesh.load_texture("assets/african_head_diffuse.tga");
    obj->mesh.load_normal_map("assets/african_head_nm_tangent.tga");

    // UI Callback
    renderer.add_ui_callback([&]() {
        ImGui::Begin("Physics Engine");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::Checkbox("Enable Physics", &renderer.physics_enabled);
        
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
