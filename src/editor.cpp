
// @Cleanup: Not sure if these should be here...
FUNCTION Point_Light default_point_light()
{
    Point_Light result = {};
    result.position    = {};
    result.intensity   = 1.0f;
    result.color       = {1.0f, 1.0f, 1.0f};
    result.range       = 3.5f;
    return result;
}
FUNCTION Directional_Light default_dir_light()
{
    Directional_Light result = {};
    result.direction                   = {-1.0f, -1.0f, -1.0f};
    result.intensity                   = 1.0f;
    result.color                       = {1.0f, 1.0f, 1.0f};
    result.indirect_lighting_intensity = 1.0f;
    return result;
}

FUNCTION void draw_main_editor_window()
{
    const char *modes[] = {"Entity Mode", "Point Light Mode", "Directional Light Mode"};
    LOCAL_PERSIST s32 current_mode = 0;
    
    ImGui::Begin("Light info", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Combo("Choose mode", &current_mode, modes, ARRAY_COUNT(modes));
    
    ImGui::Separator();
    
    switch (current_mode) {
        case 0: {
            //
            // Entity Mode
            //
            
            Entity_Manager *manager = &game->entity_manager;
            
            if (ImGui::Button("Create")) {
                Entity entity = create_default_entity();
                create_entity_and_select(manager, entity);
            }
            
            ImGui::Separator();
            
            LOCAL_PERSIST char name[128] = {};
            if (manager->selected_entity) {
                Entity *e = manager->selected_entity;
                
                MEMORY_ZERO_ARRAY(name);
                ASSERT(ARRAY_COUNT(name) > e->name.count + 1);
                MEMORY_COPY(name, e->name.data, e->name.count);
                
                // For now only display info.
                ImGui::Text("%s (%u)", name, e->idx);
                
                ImGui::Separator();
                
                //
                // Position
                //
                if (ImGui::DragFloat3("position", e->position.I, 0.005f, F32_MIN, F32_MAX, "%.3f")) {
                }
                
                //
                // Orientation (just display, use gizmo to modify).
                //
                V3  axis  = quaternion_get_axis(e->orientation);
                f32 angle = quaternion_get_angle(e->orientation);
                ImGui::Text("Axis: [%.3f, %.3f, %.3f], Angle: %.3f", axis.x, axis.y, axis.y, angle);
                
                //
                // Scale
                //
                /* 
                                LOCAL_PERSIST f32 scale = 1.0f;
                                scale = e->scale.x;
                                if (ImGui::DragFloat("Scale", &scale, 0.005f, 1.0f, F32_MAX, "%.3f")) {
                                    e->scale = v3(scale);	
                                }
                 */
                if (ImGui::DragFloat3("Scale", e->scale.I, 0.005f, 1.0f, F32_MAX, "%.3f")) {
                }
                
                //
                // Mesh
                //
                LOCAL_PERSIST s32 item_current_idx = 0;
                if (!str8_empty(manager->selected_entity->mesh->name)) {
                    const u8* combo_preview_value = manager->selected_entity->mesh->name.data;
                    if (ImGui::BeginCombo("Mesh", (const char*)combo_preview_value)) {
                        for (s32 n = 0; n < game->mesh_catalog.items.count; n++) {
                            const bool is_selected = (item_current_idx == n);
                            if (ImGui::Selectable((const char*) game->mesh_catalog.items[n]->name.data, is_selected)) {
                                item_current_idx = n;
                                
                                e->mesh = game->mesh_catalog.items[n];
                            }
                            
                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                
            }
            
        } break;
        case 1: {
            //
            // Point Light Mode
            //
            
            ImGui::SliderInt("Number of point lights", &game->num_point_lights, 0, MAX_POINT_LIGHTS, "%d", ImGuiSliderFlags_AlwaysClamp);
            
            LOCAL_PERSIST s32 light_index = 0;
            if (game->num_point_lights > 0) {
                ImGui::SliderInt("Light (index) to edit", &light_index, 0, game->num_point_lights-1, "%d", ImGuiSliderFlags_AlwaysClamp);
                
                Point_Light *light = game->point_lights + light_index;
                
                if (ImGui::Button("Default"))
                    *light = default_point_light();
                if (ImGui::Button("Clear"))
                    *light = {};
                
                ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
                
                ImGui::DragFloat3("position", light->position.I, 0.005f, F32_MIN, F32_MAX, "%.3f");
                ImGui::DragFloat("intensity (unitless)", &light->intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
                ImGui::ColorEdit3("color", light->color.I);
                ImGui::DragFloat("range", &light->range, 0.005f, 0.0f, F32_MAX, "%.3f");
                
                immediate_begin();
                set_texture(0);
                // Draw point light position and radius.
                immediate_cube(light->position, 0.2f, v4(light->color, 1.0f));
                immediate_torus(light->position, light->range, V3U, v4(1), 0.01f);
                immediate_torus(light->position, light->range, V3R, v4(1), 0.01f);
                immediate_torus(light->position, light->range, V3F, v4(1), 0.01f);
                immediate_end();
            }
        } break;
        case 2: {
            //
            // Directional Light Mode
            //
            
            ImGui::SliderInt("Number of dir lights", &game->num_dir_lights, 0, MAX_DIR_LIGHTS, "%d", ImGuiSliderFlags_AlwaysClamp);
            
            LOCAL_PERSIST s32 light_index = 0;
            if (game->num_dir_lights > 0) {
                ImGui::SliderInt("Light (index) to edit", &light_index, 0, game->num_dir_lights-1, "%d", ImGuiSliderFlags_AlwaysClamp);
                
                Directional_Light *light = game->dir_lights + light_index;
                
                if (ImGui::Button("Default"))
                    *light = default_dir_light();
                if (ImGui::Button("Clear"))
                    *light = {};
                
                ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
                
                ImGui::DragFloat3("direction", light->direction.I, 0.005f, F32_MIN, F32_MAX, "%.3f");
                ImGui::DragFloat("intensity (unitless)", &light->intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
                ImGui::ColorEdit3("color", light->color.I);
                ImGui::DragFloat("indirect intensity (unitless)", &light->indirect_lighting_intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
                
                immediate_begin();
                set_texture(0);
                immediate_arrow(v3(0), light->direction, 1.5f, v4(light->color, 0.6f));
                immediate_end();
            }
        } break;
    }
    
    ImGui::End();
}

FUNCTION void draw_editor()
{
    LOCAL_PERSIST bool show_demo = FALSE;
    if (show_demo)
        ImGui::ShowDemoWindow(&show_demo);
    
    draw_main_editor_window();
}