
FUNCTION void draw_entity_info()
{
    ImGui::Begin("Entity info", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::Button("Create")) {
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
    
    
    ImGui::End();
}

FUNCTION void draw_light_info()
{
    ImGui::Begin("Light info", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
    
    ImGui::Text("Choose type of light to edit");
    LOCAL_PERSIST s32 light_type;
    ImGui::RadioButton("Point",       &light_type, 0);
    ImGui::RadioButton("Directional", &light_type, 1);
    
    ImGui::Separator();
    
    if (light_type == 0) {
        // Point light.
        ImGui::SliderInt("Number of point lights", &game->num_point_lights, 0, MAX_POINT_LIGHTS, "%d", ImGuiSliderFlags_AlwaysClamp);
        
        for (s32 i = 0; i < game->num_point_lights; i++) {
            ImGui::Separator();
            
            Point_Light *light = game->point_lights + i;
            ImGui::DragFloat3("position", light->position.I, 0.005f, F32_MIN, F32_MAX, "%.3f");
            ImGui::DragFloat("intensity (unitless)", &light->intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
            ImGui::ColorEdit3("color", light->color.I);
            ImGui::DragFloat("attenuation radius", &light->attenuation_radius, 0.005f, 0.0f, F32_MAX, "%.3f");
        }
        
    } else if (light_type == 1) {
        // Directional light.
        ImGui::SliderInt("Number of dir lights", &game->num_dir_lights, 0, MAX_DIR_LIGHTS, "%d", ImGuiSliderFlags_AlwaysClamp);
        
        for (s32 i = 0; i < game->num_dir_lights; i++) {
            ImGui::Separator();
            
            Directional_Light *light = game->dir_lights + i;
            ImGui::DragFloat3("direction", light->direction.I, 0.005f, F32_MIN, F32_MAX, "%.3f");
            ImGui::DragFloat("intensity (unitless)", &light->intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
            ImGui::ColorEdit3("color", light->color.I);
            ImGui::DragFloat("indirect intensity (unitless)", &light->indirect_lighting_intensity, 0.005f, 0.0f, F32_MAX, "%.3f");
        }
    }
    
    
    ImGui::End();
}

FUNCTION void draw_editor()
{
    LOCAL_PERSIST bool show_demo = TRUE;
    if (show_demo)
        ImGui::ShowDemoWindow(&show_demo);
    
    draw_light_info();
    draw_entity_info();
}