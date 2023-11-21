
FUNCTION void draw_editor()
{
    LOCAL_PERSIST bool show_demo = TRUE;
    if (show_demo)
        ImGui::ShowDemoWindow(&show_demo);
}