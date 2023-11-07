
//#define INCLUDE_WASAPI
//#define INCLUDE_XINPUT
#include "win32_base.cpp"

#define ORH_STATIC
#define ORH_IMPLEMENTATION
#include "orh.h"
#include "orh_d3d11.cpp"

#include "game.cpp"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                   LPSTR command_line, int show_code)
{
    win32_init();
    win32_create_window(1600, 900, "APP", instance);
    
    win32_os_state_init();
    d3d11_init(primary_window);
    
    ShowWindow(primary_window, SW_SHOWDEFAULT);
    win32_show_cursor();
    
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
        //print("FPS: %d\n", (s32)(1.0/seconds_elapsed_for_frame));
        
        win32_update_drawing_region();
        win32_process_inputs();
        while (accumulator >= os->dt) {
            //
            // Update tick.
            //
            game_update();
            
            accumulator -= os->dt;
            os->time    += os->dt;
            
            clear_key_states();
        }
        
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
            
            d3d11_present(global_os.vsync);
        }
        
        win32_limit_framerate(last_counter);
    }
    
    return 0;
}