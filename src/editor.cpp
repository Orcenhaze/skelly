
// @Cleanup: Not sure if these should be here...
FUNCTION Point_Light default_point_light()
{
    Point_Light result = {};
    result.position    = {};
    result.intensity   = 1.0f;
    result.color       = {1.0f, 1.0f, 1.0f};
    result.range       = 0.1f;
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
            
            if (ImGui::Button("Create")) {
                // @Hardcoded?
                Entity *entity = array_add(&game->entity_manager.entities, create_default_entity());
                game->selected_entity.ptr = entity;
            }
            
            ImGui::Separator();
            
            LOCAL_PERSIST char name[128] = {};
            if (game->selected_entity.ptr) {
                Entity *e = game->selected_entity.ptr;
                
                MEMORY_ZERO_ARRAY(name);
                ASSERT(ARRAY_COUNT(name) > e->name.count + 1);
                MEMORY_COPY(name, e->name.data, e->name.count);
                
                // For now only display info.
                ImGui::Text("%s", name);
                if (ImGui::InputFloat3("position", e->position.I))        {update_entity_transform(e);}
                // @Todo: Use axis angle.
                if (ImGui::InputFloat4("Orientation", e->orientation.I)) {update_entity_transform(e);}
                if (ImGui::InputFloat3("scale", e->scale.I))             {update_entity_transform(e);}
                
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