
#if DEVELOPER
#define IMGUI_STB_TRUETYPE_FILENAME "stb/stb_truetype.h"
#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#include "imgui/imgui.cpp"
#include "imgui/imgui_impl_win32.cpp"
#include "imgui/imgui_impl_dx11.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"
#endif

//#define INCLUDE_WASAPI
//#define INCLUDE_XINPUT
#include "win32_base.cpp"

#define ORH_STATIC
#define ORH_IMPLEMENTATION
#include "orh.h"
#include "orh_d3d11.cpp"
#include "orh_collision.h"

#include "game.h"
#include "game.cpp"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                   LPSTR command_line, int show_code)
{
    win32_init();
    HWND window = win32_create_window(1600, 900, "APP", instance);
    
    win32_os_state_init(window);
    d3d11_init(window);
    
    ShowWindow(window, SW_SHOWDEFAULT);
    UpdateWindow(window);
    win32_show_cursor();
    
#if DEVELOPER
    //
    // Setup Dear ImGui context and renderer.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.0f;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);
#endif
    
    game_init();
    
    LARGE_INTEGER last_counter = win32_qpc();
    f64 accumulator = 0.0;
    while (!global_os.exit) {
        // Sleep until our Present call won't block for queuing up a new frame. 
        d3d11_wait_for_swapchain();
        
        // Frame timing/end.
        // @Note: Look at Phillip Trudeau's diagram on discord.
        LARGE_INTEGER end_counter     = win32_qpc();
        f64 seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, end_counter);
        last_counter                  = end_counter;
        accumulator                  += seconds_elapsed_for_frame;
        accumulator                   = CLAMP_UPPER(0.1, accumulator);
        //debug_print("FPS: %d\n", (s32)(1.0/seconds_elapsed_for_frame));
        
        win32_update_drawing_region(window);
        win32_process_inputs(window);
        
        while (accumulator >= os->dt) {
            //
            // Update tick.
            //
            game_update();
            
            accumulator -= os->dt;
            os->time    += os->dt;
            
            clear_key_states();
        }
        
#if DEVELOPER
        // Start the Dear ImGui frame.
        if ((global_os.window_size.x != 0) && (global_os.window_size.y != 0)) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }
#endif
        // Render (only if window size is non-zero).
        if ((global_os.window_size.x != 0) && (global_os.window_size.y != 0)) {
            d3d11_viewport(global_os.drawing_rect.min.x, 
                           global_os.drawing_rect.min.y, 
                           get_width(global_os.drawing_rect), 
                           get_height(global_os.drawing_rect));
            d3d11_clear();
            
            //
            // Render stuff.
            //
            game_render();
            
#if DEVELOPER
            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
            d3d11_present(global_os.vsync);
        }
        
        win32_limit_framerate(last_counter);
    }
    
#if DEVELOPER
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
    
    return 0;
}