/* win32_base.cpp - v0.04 - base functionality for win32_main.cpp

REVISION HISTORY:
0.04 - added some cursor functionality and cleaned some stuff up.
0.03 - removed primary_window; now we pass HWND to functions that need it.
0.02 - renamed print() to debug_print() and added ability to load File_Info and File_Group and string stuff.
0.01 - added mouse capture for middle mouse button.

*/

////////////////////////////////
//~ Includes
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef INCLUDE_WASAPI
#include "win32_wasapi.h"
#pragma warning(push, 3)
#pragma warning(disable: 4701)
#include "stb/stb_vorbis.c"    // For loading ogg vorbis audio files.
#pragma warning(pop)
GLOBAL Wasapi_Audio audio;
#endif

#ifdef INCLUDE_XINPUT
#include "win32_xinput.h"
#endif

#include "orh.h"

// Tell AMD and Nvidia drivers to use the most powerful GPU instead of a lower-performance (such as integrated) GPU.
// App should use either DirectX or OpenGL for this to work.
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement                  = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

// For registering raw input devices.
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC  ((unsigned short) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((unsigned short) 0x02)
#endif

////////////////////////////////
//~ Globals
struct Win32
{
    OS_State state;
    char     exe_full_path    [256];
    char     exe_parent_folder[256];
    char     data_folder      [256];
    //char     saved_games_folder[256];
    HWND     window;                // Handle to the window we created.
    HANDLE   waitable_timer;        // Used for frame limiter.
    s64      performance_frequency; // Used timing things with QueryPerformanceCounter.
    b32      keep_cursor_centered;
    s32      restore_cursor_pos_x, restore_cursor_pos_y;
};
GLOBAL Win32 _win32;

////////////////////////////////
//~ Timing
inline LARGE_INTEGER win32_qpc()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline f64 win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    // Make sure we QueryPerformanceFrequency() before calling this (we do in win32_init).
    ASSERT(_win32.performance_frequency != 0);
    
    f64 result = (f64)(end.QuadPart - start.QuadPart) / (f64)_win32.performance_frequency;
    return result;
}

////////////////////////////////
//~ Utility
FUNCTION void win32_toggle_fullscreen(HWND window)
{
    LOCAL_PERSIST WINDOWPLACEMENT last_window_placement = {
        sizeof(last_window_placement)
    };
    
    DWORD window_style = GetWindowLong(window, GWL_STYLE);
    if(window_style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        if(GetWindowPlacement(window, &last_window_placement) &&
           GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY),
                          &monitor_info))
        {
            
            SetWindowLong(window, GWL_STYLE,
                          window_style & ~WS_OVERLAPPEDWINDOW);
            
            SetWindowPos(window, HWND_TOP,
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right -
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom -
                         monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE,
                      window_style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &last_window_placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

FUNCTION void win32_print_to_debug_output(String8 text)
{
    OutputDebugStringA((LPCSTR)text.data);
}

FUNCTION void win32_build_paths()
{
    GetModuleFileNameA(0, _win32.exe_full_path, sizeof(_win32.exe_full_path));
    
    char *one_past_slash = _win32.exe_full_path;
    char *exe            = _win32.exe_full_path;
    while (*exe)  {
        if (*exe++ == '\\') one_past_slash = exe;
    }
    MEMORY_COPY(_win32.exe_parent_folder, _win32.exe_full_path, one_past_slash - _win32.exe_full_path);
    
#if DEVELOPER
    string_format(_win32.data_folder, sizeof(_win32.data_folder), "%s../data/", _win32.exe_parent_folder);
#else
    // @Note: We will copy the data folder when building the game and put it next to .exe.
    string_format(_win32.data_folder, sizeof(_win32.data_folder), "%sdata/", _win32.exe_parent_folder);
#endif
    
#if 0
    // Store saved games folder.
    PWSTR wide_path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, 0, &wide_path))) {
        // Convert the wide character path to a ascii/ansi
        int size = WideCharToMultiByte(CP_ACP, 0, wide_path, -1, 0, 0, 0, 0);
        if (size > 0)
            WideCharToMultiByte(CP_ACP, 0, wide_path, -1, _win32.saved_games_folder, size, 0, 0);
        
        // Remember to free the allocated memory for the wide character path
        CoTaskMemFree(wide_path);
    }
#endif
}

FUNCTION void win32_show_window(HWND window)
{
    ShowWindow(window, SW_SHOWDEFAULT);
}

FUNCTION void win32_focus_window(HWND window)
{
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetFocus(window);
}

FUNCTION void win32_get_cursor_position(HWND window, s32 *x, s32 *y)
{
    // Return pos in client window space.
    POINT pos; 
    if (GetCursorPos(&pos)) {
        ScreenToClient(window, &pos);
        if (x) *x = pos.x;
        if (y) *y = pos.y;
    }
}
FUNCTION void win32_set_cursor_position(HWND window, s32 x, s32 y)
{
    // Pass pos in client window space.
    POINT pos = {x, y};
    ClientToScreen(window, &pos);
    SetCursorPos(pos.x, pos.y);
}

FUNCTION void win32_show_cursor()
{
    while (ShowCursor(TRUE) < 0);
}
FUNCTION void win32_hide_cursor()
{
    while (ShowCursor(FALSE) >= 0);
}

FUNCTION void win32_capture_cursor()
{
    // Confine cursor position within client window rect.
    RECT clip_rect;
    GetClientRect(_win32.window, &clip_rect);
    ClientToScreen(_win32.window, (POINT*) &clip_rect.left);
    ClientToScreen(_win32.window, (POINT*) &clip_rect.right);
    ClipCursor(&clip_rect);
}
FUNCTION void win32_release_cursor()
{
    ClipCursor(NULL);
}

FUNCTION void win32_center_cursor()
{
    // Set cursor pos to center of window rect. 
    RECT window_rect;
    GetClientRect(_win32.window, &window_rect);
    ClientToScreen(_win32.window, (POINT*) &window_rect.left);
    ClientToScreen(_win32.window, (POINT*) &window_rect.right);
    s32 x = window_rect.left + (s32)(_win32.state.window_size.x * 0.5f);
    s32 y = window_rect.top  + (s32)(_win32.state.window_size.y * 0.5f);
    SetCursorPos(x, y);
}

FUNCTION void win32_disable_cursor()
{
    win32_get_cursor_position(_win32.window,
                              &_win32.restore_cursor_pos_x,
                              &_win32.restore_cursor_pos_y);
    win32_hide_cursor();
    win32_center_cursor();
    _win32.keep_cursor_centered = TRUE;
    win32_capture_cursor();
}
FUNCTION void win32_enable_cursor()
{
    win32_release_cursor();
    _win32.keep_cursor_centered = FALSE;
    win32_show_cursor();
    win32_set_cursor_position(_win32.window,
                              _win32.restore_cursor_pos_x,
                              _win32.restore_cursor_pos_y);
}

////////////////////////////////
//~ File IO
FUNCTION void win32_free_file_memory(void *memory)
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

FUNCTION String8 win32_read_entire_file(String8 full_path)
{
    String8 result = {};
    
    HANDLE file_handle = CreateFile((char*)full_path.data, GENERIC_READ, FILE_SHARE_READ, 0, 
                                    OPEN_EXISTING, 0, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        debug_print("OS Error: read_entire_file() INVALID_HANDLE_VALUE!\n");
        return result;
    }
    
    LARGE_INTEGER file_size64;
    if (GetFileSizeEx(file_handle, &file_size64) == 0) {
        debug_print("OS Error: read_entire_file() GetFileSizeEx() failed!\n");
    }
    
    u32 file_size32 = (u32)file_size64.QuadPart;
    result.data = (u8 *) VirtualAlloc(0, file_size32, MEM_RESERVE|MEM_COMMIT, 
                                      PAGE_READWRITE);
    
    if (!result.data) {
        debug_print("OS Error: read_entire_file() VirtualAlloc() returned 0!\n");
    }
    
    DWORD bytes_read;
    if (ReadFile(file_handle, result.data, file_size32, &bytes_read, 0) && (file_size32 == bytes_read)) {
        result.count = file_size32;
    } else {
        debug_print("OS Error: read_entire_file() ReadFile() failed!\n");
        
        win32_free_file_memory(result.data);
        result.data = 0;
    }
    
    CloseHandle(file_handle);
    
    return result;
}

FUNCTION b32 win32_write_entire_file(String8 full_path, String8 data)
{
    b32 result = FALSE;
    
    HANDLE file_handle = CreateFile((char*)full_path.data, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        debug_print("OS Error: write_entire_file() INVALID_HANDLE_VALUE!\n");
    }
    
    DWORD bytes_written;
    if (WriteFile(file_handle, data.data, (DWORD)data.count, &bytes_written, 0) && (bytes_written == data.count)) {
        result = TRUE;
    } else {
        debug_print("OS Error: write_entire_file() WriteFile() failed!\n");
    }
    
    CloseHandle(file_handle);
    
    return result;
}

FUNCTION File_Group win32_get_all_files_in_path(Arena *arena, String8 path_wildcard)
{
    File_Group result = {};
    
    String8 directory = extract_parent_folder(path_wildcard);
    
    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFile((char*)path_wildcard.data, &find_data); 
    while(find_handle != INVALID_HANDLE_VALUE)
    {
        File_Info *info = PUSH_STRUCT_ZERO(arena, File_Info);
        
        info->next      = result.first_file_info;
        info->file_date = (((u64)find_data.ftLastWriteTime.dwHighDateTime << (u64)32) | (u64)find_data.ftLastWriteTime.dwLowDateTime);
        info->file_size = (((u64)find_data.nFileSizeHigh << (u64)32) | (u64)find_data.nFileSizeLow);
        
        String8 file_name = str8_cstring(find_data.cFileName); // Includes extension.
        
        info->full_path = str8_cat(arena, directory, file_name);
        info->base_name = str8_copy(arena, extract_base_name(file_name));
        
        result.first_file_info = info;
        result.file_count++;
        
        if(!FindNextFile(find_handle, &find_data)) break;
    }
    
    return result;
}

////////////////////////////////
//~ Memory
FUNCTION void* win32_reserve(u64 size)
{
    void *memory = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return memory;
}

FUNCTION void  win32_release(void *memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

FUNCTION b32  win32_commit(void *memory, u64 size)
{
    b32 result = (VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return result;
}

FUNCTION void  win32_decommit(void *memory, u64 size)
{
    VirtualFree(memory, size, MEM_DECOMMIT);
}

////////////////////////////////
//~ Message/input processing
FUNCTION void win32_process_pending_messages(HWND window)
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch (message.message) {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                s32 vkcode   = (s32) message.wParam;
                b32 alt_down = (message.lParam & (1 << 29));
                b32 was_down = (message.lParam & (1 << 30)) == 1;
                b32 is_down  = (message.lParam & (1 << 31)) == 0;
                
                if((vkcode == VK_F4) && alt_down)
                {
                    _win32.state.exit = TRUE;
                    return;
                }
                
                s32 key = Key_NONE;
                if ((vkcode >= 'A') && (vkcode <= 'Z')) {
                    key = Key_A + (vkcode - 'A');
                } else if ((vkcode >= '0') && (vkcode <= '9')) {
                    key = Key_0 + (vkcode - '0');
                } else {
                    if     ((vkcode >= VK_F1) && (vkcode <= VK_F12)) key = Key_F1 + (vkcode - VK_F1);
                    else if (vkcode == VK_ESCAPE)  key = Key_ESCAPE;
                    else if (vkcode == VK_BACK)    key = Key_BACKSPACE;
                    else if (vkcode == VK_TAB)     key = Key_TAB;
                    else if (vkcode == VK_RETURN)  key = Key_ENTER;
                    else if (vkcode == VK_SHIFT)   key = Key_SHIFT;
                    else if (vkcode == VK_CONTROL) key = Key_CONTROL;
                    else if (vkcode == VK_MENU)    key = Key_ALT;
                    else if (vkcode == VK_SPACE)   key = Key_SPACE;
                    else if (vkcode == VK_LEFT)    key = Key_LEFT;
                    else if (vkcode == VK_UP)      key = Key_UP;
                    else if (vkcode == VK_RIGHT)   key = Key_RIGHT;
                    else if (vkcode == VK_DOWN)    key = Key_DOWN;
                }
                
                if (key != Key_NONE) {
                    Queued_Input input = {key, is_down};
                    array_add(&_win32.state.inputs_to_process, input);
                }
            } break;
            
            case WM_LBUTTONDOWN: {
                SetCapture(window);
                Queued_Input input = {Key_MLEFT, (message.wParam & MK_LBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            case WM_LBUTTONUP: {
                ReleaseCapture();
                Queued_Input input = {Key_MLEFT, (message.wParam & MK_LBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            
            case WM_RBUTTONDOWN: {
                SetCapture(window);
                Queued_Input input = {Key_MRIGHT, (message.wParam & MK_RBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            case WM_RBUTTONUP: {
                ReleaseCapture();
                Queued_Input input = {Key_MRIGHT, (message.wParam & MK_RBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            
            case WM_MBUTTONDOWN:
            {
                SetCapture(window);
                Queued_Input input = {Key_MMIDDLE, (message.wParam & MK_MBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            case WM_MBUTTONUP: {
                ReleaseCapture();
                Queued_Input input = {Key_MMIDDLE, (message.wParam & MK_MBUTTON)};
                array_add(&_win32.state.inputs_to_process, input);
            } break;
            
            // @Note: Process registered raw input devices!
            case WM_INPUT: {
                UINT size = sizeof(RAWINPUT);
                LOCAL_PERSIST RAWINPUT raw[sizeof(RAWINPUT)];
                
                GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
                
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    // This is the raw mouse detla.
                    s32 raw_x = raw->data.mouse.lLastX;
                    s32 raw_y = raw->data.mouse.lLastY;
                    
                    _win32.state.tick_input.mouse_delta_raw  = {raw_x, raw_y};
                    _win32.state.tick_input.mouse_delta      = v2((f32)raw_x, (f32)raw_y) * BASE_MOUSE_RESOLUTION;
                    
                    _win32.state.frame_input.mouse_delta_raw = {raw_x, raw_y};
                    _win32.state.frame_input.mouse_delta     = v2((f32)raw_x, (f32)raw_y) * BASE_MOUSE_RESOLUTION;
                    
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                        s32 raw_wheel = (*(short*)&raw->data.mouse.usButtonData) / WHEEL_DELTA;
                        _win32.state.tick_input.mouse_wheel_raw = raw_wheel;
                        _win32.state.frame_input.mouse_wheel_raw = raw_wheel;
                    }
                }
            } break;
            
#if 0
            // No longer used. Using raw input instead.
            case WM_MOUSEWHEEL: {
                f32 wheel_delta        = (f32) GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                _win32.state.mouse_scroll = v2(0, wheel_delta);
            } break;
            case WM_MOUSEHWHEEL: {
                f32 wheel_delta        = (f32) GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                _win32.state.mouse_scroll = v2(wheel_delta, 0);
            } break;
#endif
            
            default:
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }
}

FUNCTION LRESULT CALLBACK win32_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
#if DEVELOPER
    if (ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam))
        return TRUE;
#endif
    
    LRESULT result = 0;
    
    switch (message) {
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP: {
            clear_input(&_win32.state.tick_input);
            clear_input(&_win32.state.frame_input);
        } break;
        
        case WM_CLOSE: 
        case WM_DESTROY:
        case WM_QUIT: {
            _win32.state.exit = TRUE;
        } break;
        
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

FUNCTION void win32_process_inputs(HWND window)
{
    // Mouse position.
    //
    s32 cursor_x, cursor_y;
    win32_get_cursor_position(window, &cursor_x, &cursor_y);
    V2 mouse_client = 
    {
        (f32) cursor_x,
        ((f32)_win32.state.window_size.h - 1.0f) - cursor_y,
    };
    _win32.state.mouse_ndc = 
    {
        2.0f*CLAMP01_RANGE(_win32.state.drawing_rect.min.x, mouse_client.x, _win32.state.drawing_rect.max.x) - 1.0f,
        2.0f*CLAMP01_RANGE(_win32.state.drawing_rect.min.y, mouse_client.y, _win32.state.drawing_rect.max.y) - 1.0f,
        0.0f
    };
    
#if DEVELOPER
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //debug_print("Want keyboard? %b     Want mouse? %b \n", io.WantCaptureKeyboard, io.WantCaptureMouse);
    if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
        clear_input(&_win32.state.tick_input);
        clear_input(&_win32.state.frame_input);
        
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        return;
    }
#endif
    
    // Put input messages in queue + process raw input.
    //
    win32_process_pending_messages(window);
    
    // Process queued inputs.
    //
    for (s32 i = 0; i < _win32.state.inputs_to_process.count; i++) {
        Queued_Input input = _win32.state.inputs_to_process[i];
        if (input.down) {
            //
            // @Todo: This order preserving stuff could be wrong when we introduced per-frame input.
            //
            
            // Defer DOWN event because UP was TRUE, break to preserve order.
            if (_win32.state.tick_input.released[input.key] && _win32.state.frame_input.released[input.key])
                break;
            // Defer DOWN event because DOWN is already TRUE, break to preserve order.
            else if (_win32.state.tick_input.pressed[input.key] && _win32.state.frame_input.pressed[input.key])
                break;
            else {
                // Set tick input.
                if (!_win32.state.tick_input.held[input.key])
                    _win32.state.tick_input.pressed[input.key] = TRUE;
                _win32.state.tick_input.held[input.key] = TRUE;
                
                // Set frame input.
                if (!_win32.state.frame_input.held[input.key])
                    _win32.state.frame_input.pressed[input.key] = TRUE;
                _win32.state.frame_input.held[input.key] = TRUE;
                
                array_ordered_remove_by_index(&_win32.state.inputs_to_process, i);
                i--;
            }
        } else {
            // Defer UP event because DOWN was TRUE, break to preserve order.
            if (_win32.state.tick_input.pressed[input.key] && _win32.state.frame_input.pressed[input.key])
                break;
            // Defer UP event because UP is already TRUE, break to preserve order.
            else if (_win32.state.tick_input.released[input.key] && _win32.state.frame_input.released[input.key])
                break;
            else {
                // Set tick input.
                _win32.state.tick_input.released[input.key] = TRUE;
                _win32.state.tick_input.held[input.key]     = FALSE;
                
                // Set frame_input.
                _win32.state.frame_input.released[input.key] = TRUE;
                _win32.state.frame_input.held[input.key]     = FALSE;
                
                array_ordered_remove_by_index(&_win32.state.inputs_to_process, i);
                i--;
            }
        }
    }
    
#ifdef INCLUDE_XINPUT
    // Do same thing for gamepads... Get gamepad state, then encode button states how we want.
    //
    for (s32 i = 0; i < GAMEPADS_MAX; i++) {
        XInput_Gamepad xinput_pad = xinput_get_gamepad_state(i);
        if (!xinput_pad.connected)
            continue;
        
        Gamepad *pad       = &_win32.state.gamepads[i];
        pad->connected     = xinput_pad.connected;
        pad->left_stick    = {xinput_pad.stick_left_x, xinput_pad.stick_left_y};
        pad->right_stick   = {xinput_pad.stick_right_x, xinput_pad.stick_right_y};
        pad->left_trigger  = xinput_pad.trigger_left;
        pad->right_trigger = xinput_pad.trigger_right;
        
        // Process xinput button states and encode them the way we want.
        //
        for (s32 j = 1; j < XInputButton_COUNT; j++) {
            s32 button = j;
            b32 down   = xinput_pad.button_states[button];
            if (down) {
                if (!pad->held[button])
                    pad->pressed[button] = TRUE;
                
                pad->held[button] = TRUE;
            } else {
                if (pad->held[button])
                    pad->released[button] = TRUE;
                
                pad->held[button] = FALSE;
            }
        }
    }
#endif
}

////////////////////////////////
//~ Win32 initialize
FUNCTION void win32_init()
{
    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);
    _win32.performance_frequency = performance_frequency.QuadPart;
    
    _win32.waitable_timer = CreateWaitableTimerExW(0, 0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    ASSERT(_win32.waitable_timer != 0);
    
    win32_build_paths();
}

FUNCTION void win32_register_raw_input_devices(HWND window)
{
    // We're configuring just one RAWINPUTDEVICE, the mouse,
	// so it's a single-element array (a pointer).
	RAWINPUTDEVICE rid[1];
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage     = HID_USAGE_GENERIC_MOUSE;
	rid[0].dwFlags     = RIDEV_INPUTSINK;
	rid[0].hwndTarget  = window;
	RegisterRawInputDevices(rid, 1, sizeof(rid[0]));
}

FUNCTION HWND win32_create_window(int window_width, int window_height, LPCSTR window_name, HINSTANCE instance)
{
    //
    // Register window_class.
    WNDCLASS window_class = {};
    
    window_class.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc   = win32_wndproc;
    window_class.hInstance     = instance;
    window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //window_class.hIcon;
    window_class.lpszClassName = "MainWindowClass";
    
    if (!RegisterClassA(&window_class)) {
        OutputDebugStringA("Could not register window class.\n");
        ExitProcess(0);
    }
    
    //
    // Create window.
    HWND window = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP, //WS_EX_TOPMOST,
                                 window_class.lpszClassName,
                                 window_name,
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 window_width,
                                 window_height,
                                 0,
                                 0,
                                 instance,
                                 0);
    
    if (!window) {
        OutputDebugStringA("Window creation failed.\n");
        ExitProcess(0);
    }
    
    _win32.window = window;
    
    win32_init();
    
    win32_register_raw_input_devices(window);
    
    win32_show_window(window);
    win32_focus_window(window);
    
#if DEVELOPER
    win32_show_cursor();
#else
    win32_hide_cursor();
#endif
    
    return window;
}

////////////////////////////////
//~ WASAPI
#ifdef INCLUDE_WASAPI
FUNCTION void win32_wasapi_init(size_t sample_rate = 48000, size_t channel_count = 2, DWORD channel_mask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT)
{
    wasapi_start(&audio, sample_rate, 2, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
}

FUNCTION Sound win32_sound_load(String8 full_path, u32 sample_rate)
{
    // Load mono audio with specified samplerate.
    
    String8 file = win32_read_entire_file(full_path);
    if (!file.data) {
        debug_print("Couldn't load sound file %S\n", full_path);
        
        Sound dummy = {};
        return dummy;
    }
    
    s16 *out;
    s32 chan, samplerate;
    s32 count = stb_vorbis_decode_memory(file.data, (s32)file.count, &chan, &samplerate, &out);
    
    if (count == -1) {
        debug_print("STB Error: Couldn't load sound file %S\n", full_path);
        
        Sound dummy = {};
        return dummy;
    }
    
    // Make sure we are loading mono audio with specified sample rate.
    ASSERT((chan == 1) && ((u32)samplerate == sample_rate));
    
    Sound result;
    result.samples = out;
    result.count   = result.pos = (u32)count;
    
    return result;
}

FUNCTION void win32_wasapi_begin()
{
    wasapi_lock_buffer(&audio);
    _win32.state.samples_out        = (f32 *)audio.sample_buffer;
    _win32.state.samples_to_write   = (u32)MIN(_win32.state.sample_rate/10, audio.sample_count);
    _win32.state.samples_to_advance = (u32)audio.num_samples_submitted;
    MEMORY_ZERO(_win32.state.samples_out, _win32.state.samples_to_write * _win32.state.bytes_per_sample);
}

FUNCTION void win32_wasapi_end()
{
    wasapi_unlock_buffer(&audio, _win32.state.samples_to_write);
}

FUNCTION void win32_wasapi_free()
{
    wasapi_stop(&audio);
}
#endif

////////////////////////////////
//~ OS_State init
FUNCTION void win32_os_state_init(HWND window)
{
    os = &_win32.state;
    
    // Meta-data.
    _win32.state.exe_full_path     = str8_cstring(_win32.exe_full_path);
    _win32.state.exe_parent_folder = str8_cstring(_win32.exe_parent_folder);
    _win32.state.data_folder       = str8_cstring(_win32.data_folder);
    
    // Options.
#if DEVELOPER
    _win32.state.fullscreen        = FALSE;
#else
    _win32.state.fullscreen        = TRUE;
    win32_toggle_fullscreen(window);
#endif
    _win32.state.exit              = FALSE;
    
    _win32.state.vsync             = TRUE;
    _win32.state.fix_aspect_ratio  = TRUE;
    _win32.state.render_size       = {1920, 1080};
    _win32.state.dt                = 1.0f/120.0f;
    _win32.state.fps_max           = 120;
    
    // @Note: Force fps_max to primary monitor refresh rate if possible.
    s32 refresh_hz = GetDeviceCaps(GetDC(window), VREFRESH);
    if ((!_win32.state.vsync) && (_win32.state.fps_max > 0) && (refresh_hz > 1))
        _win32.state.fps_max = refresh_hz;
    
    _win32.state.time              = 0.0f;
    
    // Functions.
    _win32.state.disable_cursor        = win32_disable_cursor;
    _win32.state.enable_cursor         = win32_enable_cursor;
    _win32.state.reserve               = win32_reserve;
    _win32.state.release               = win32_release;
    _win32.state.commit                = win32_commit;
    _win32.state.decommit              = win32_decommit;
    _win32.state.print_to_debug_output = win32_print_to_debug_output;
    _win32.state.read_entire_file      = win32_read_entire_file;
    _win32.state.write_entire_file     = win32_write_entire_file;
    _win32.state.get_all_files_in_path = win32_get_all_files_in_path;
    _win32.state.free_file_memory      = win32_free_file_memory;
#ifdef INCLUDE_WASAPI
    _win32.state.sound_load            = win32_sound_load;
#endif
    
    // Arenas.
    _win32.state.permanent_arena  = arena_init();
    
    // User Input.
    array_init_and_reserve(&_win32.state.inputs_to_process, 512);
    
#ifdef INCLUDE_WASAPI
    // Audio Output.
    ASSERT(audio.initialized);
    _win32.state.sample_rate        = audio.buffer_format->nSamplesPerSec;
    _win32.state.bytes_per_sample   = audio.buffer_format->nBlockAlign;
    _win32.state.samples_out        = 0;
    _win32.state.samples_to_write   = 0;
    _win32.state.samples_to_advance = 0;
#endif
}

////////////////////////////////
//~ Misc
FUNCTION void win32_update_drawing_region(HWND window)
{
    RECT rect;
    GetClientRect(window, &rect);
    V2u window_size = {(u32)(rect.right - rect.left), (u32)(rect.bottom - rect.top)};
    if (_win32.state.window_size != window_size) {
        Rect2 drawing_rect;
        if (_win32.state.fix_aspect_ratio) {
            drawing_rect = aspect_ratio_fit(_win32.state.render_size, window_size);
        } else {
            drawing_rect.min = {0, 0};
            drawing_rect.max = {(f32)window_size.x, (f32)window_size.y};
        }
        
        _win32.state.window_size  = window_size;
        _win32.state.drawing_rect = drawing_rect;
    }
}

FUNCTION void win32_limit_framerate(LARGE_INTEGER last_counter)
{
    // Framerate limiter (if vsync is off).
    if ((!_win32.state.vsync) && (_win32.state.fps_max > 0)) {
        f64 target_seconds_per_frame = 1.0/(f64)_win32.state.fps_max;
        f64 seconds_elapsed_so_far = win32_get_seconds_elapsed(last_counter, win32_qpc());
        if (seconds_elapsed_so_far < target_seconds_per_frame) {
            if (_win32.waitable_timer) {
                f64 us_to_sleep = (target_seconds_per_frame - seconds_elapsed_so_far) * 1000000.0;
                us_to_sleep    -= 1000.0; // -= 1ms;
                if (us_to_sleep > 1) {
                    LARGE_INTEGER due_time;
                    due_time.QuadPart = -(s64)us_to_sleep * 10; // *10 because 100 ns intervals.
                    b32 set_ok = SetWaitableTimerEx(_win32.waitable_timer, &due_time, 0, 0, 0, 0, 0);
                    ASSERT(set_ok != 0);
                    
                    WaitForSingleObjectEx(_win32.waitable_timer, INFINITE, 0);
                }
            } else {
                // @Todo: Must use other methods of sleeping for older versions of Windows that don't have high resolution waitable timer.
            }
            
            // Spin-lock the remaining amount.
            while (seconds_elapsed_so_far < target_seconds_per_frame)
                seconds_elapsed_so_far = win32_get_seconds_elapsed(last_counter, win32_qpc());
        }
    }
}

FUNCTION void win32_update_window_events(HWND window)
{
    win32_update_drawing_region(window);
    win32_process_inputs(window);
    
    if (_win32.keep_cursor_centered)
        win32_center_cursor();
}