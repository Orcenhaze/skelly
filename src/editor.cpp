
FUNCTION void draw_main_editor_window()
{
    const char *modes[] = {"Entity Mode", "Point Light Mode", "Directional Light Mode"};
    LOCAL_PERSIST s32 current_mode = 0;
    
    ImGui::Begin("Main Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Combo("Choose mode", &current_mode, modes, ARRAY_COUNT(modes));
    
    ImGui::Separator();
    
    switch (current_mode) {
        case 0: {
            //
            // Entity Mode
            //
            
            //
            // DEBUGGING
            //
            ImGui::Checkbox("Draw Joint Lines", (bool*) &DRAW_JOINT_LINES);
            ImGui::Checkbox("Draw Joint Names", (bool*) &DRAW_JOINT_NAMES);
            
            ImGui::Separator();
            
            Entity_Manager *manager = &game->entity_manager;
            
            //
            // @Todo: Naming newly created entities.
            //
            if (ImGui::Button("Create")) {
                Entity entity = create_default_entity();
                register_new_entity(manager, entity);
            }
            
            ImGui::Separator();
            
            if (manager->selected_entity) {
                Entity *e = manager->selected_entity;
                
                // For now only display info. Allow editing of name later?
                ImGui::Text("%s ID: %u", e->name.data, e->idx);
                
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
                {
                    LOCAL_PERSIST s32 item_current_idx = 0;
                    if (!str8_empty(e->mesh->name)) {
                        const u8* combo_preview_value = e->mesh->name.data;
                        if (ImGui::BeginCombo("Mesh", (const char*)combo_preview_value)) {
                            for (s32 n = 0; n < game->mesh_catalog.items.count; n++) {
                                const bool is_selected = (item_current_idx == n);
                                if (ImGui::Selectable((const char*) game->mesh_catalog.items[n]->name.data, is_selected)) {
                                    item_current_idx = n;
                                    
                                    e->mesh = game->mesh_catalog.items[n];
                                    if (e->animation_player) {
                                        set_mesh(e->animation_player, e->mesh);
                                    }
                                }
                                
                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                }
                
                
#if 0
                // @Temporary
                // @Temporary
                // @Temporary
                // @Remove
                // @Remove
                // @Remove
                // Animation
                //
                {
                    LOCAL_PERSIST s32 item_current_idx = 0;
                    if (e->animation_player && e->mesh->skeleton) {
                        if (e->animation_player->channels.count) {
                            const u8* combo_preview_value = e->animation_player->channels[0]->animation->name.data;
                            if (ImGui::BeginCombo("Animation", (const char*)combo_preview_value)) {
                                for (s32 n = 0; n < game->animation_catalog.items.count; n++) {
                                    const bool is_selected = (item_current_idx == n);
                                    if (ImGui::Selectable((const char*) game->animation_catalog.items[n]->name.data, is_selected)) {
                                        item_current_idx = n;
                                        
                                        set_animation(e->animation_player->channels[0], game->animation_catalog.items[n], 0);
                                    }
                                    
                                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                    if (is_selected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                        } else {
                            add_animation_channel(e->animation_player);
                        }
                    }
                }
#endif
                
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

FUNCTION void draw_editor_ui()
{
    LOCAL_PERSIST bool show_demo = FALSE;
    if (show_demo)
        ImGui::ShowDemoWindow(&show_demo);
    
    draw_main_editor_window();
}