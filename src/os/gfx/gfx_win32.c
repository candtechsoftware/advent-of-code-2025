#if defined(_WIN32) || defined(_WIN64)

#    include <windows.h>
#    include <windowsx.h>
#    include <dwmapi.h>

#    include "../os_inc.h"
#    include "gfx.h"

#    pragma comment(lib, "user32.lib")
#    pragma comment(lib, "dwmapi.lib")
#    pragma comment(lib, "winmm.lib")

typedef struct Win32_Window_State Win32_Window_State;
struct Win32_Window_State {
    HWND     hwnd;
    b32      is_focused;
    b32      is_maximized;
    b32      is_fullscreen;
    b32      is_minimized;
    b32      has_close_event;
    f32      dpi_scale;
    Vec2_s32 size;
};

typedef struct Win32_State Win32_State;
struct Win32_State {
    HINSTANCE           instance;
    Win32_Window_State *windows;
    u64                 window_count;
    u64                 window_capacity;
    HCURSOR             cursors[OS_Cursor_COUNT];
    OS_Cursor           current_cursor;
    Arena              *arena;
    b32                 key_down[OS_Key_COUNT];
    OS_Modifiers        current_modifiers;
};

static Win32_State *win32_state = 0;

static OS_Key              os_key_from_win32_vkey(WPARAM vkey, LPARAM lparam);
OS_Modifiers        os_modifiers_from_win32_modifiers(void);
Win32_Window_State *win32_window_state_from_handle(OS_Handle handle);
void                win32_set_cursor(OS_Cursor cursor);

static LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (!win32_state) {
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    Win32_Window_State *window_state = (Win32_Window_State *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
		case WM_CLOSE: {
			if (window_state) {
				window_state->has_close_event = true;
			}
			return 0;
		}
		
		case WM_SIZE: {
			if (window_state) {
				RECT client_rect;
				GetClientRect(hwnd, &client_rect);
				window_state->size.x = client_rect.right - client_rect.left;
				window_state->size.y = client_rect.bottom - client_rect.top;
				
				WINDOWPLACEMENT placement = {sizeof(WINDOWPLACEMENT)};
				GetWindowPlacement(hwnd, &placement);
				window_state->is_maximized = (placement.showCmd == SW_SHOWMAXIMIZED);
				window_state->is_minimized = (placement.showCmd == SW_SHOWMINIMIZED);
			}
			return 0;
		}
		
		case WM_SETFOCUS: {
			if (window_state) {
				window_state->is_focused = true;
			}
			return 0;
		}
		
		case WM_KILLFOCUS: {
			if (window_state) {
				window_state->is_focused = false;
			}
			return 0;
		}
		
		case WM_DPICHANGED: {
			if (window_state) {
				window_state->dpi_scale = (f32)HIWORD(wparam) / 96.0f;
			}
			return 0;
		}
		
		case WM_SETCURSOR: {
			if (LOWORD(lparam) == HTCLIENT) {
				win32_set_cursor(win32_state->current_cursor);
				return TRUE;
			}
			return DefWindowProcA(hwnd, msg, wparam, lparam);
		}
		
		default:
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}

void os_gfx_init(void) {
    if (win32_state) {
        return;
    }
	
    Arena *arena = arena_alloc();
    win32_state = push_array(arena, Win32_State, 1);
    win32_state->arena = arena;
	
    win32_state->instance = GetModuleHandleA(0);
	
    // Register window class
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = win32_window_proc;
    wc.hInstance = win32_state->instance;
    wc.lpszClassName = "AppWindowClass";
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClassA(&wc);
	
    win32_state->window_capacity = 16;
    win32_state->windows = push_array(arena, Win32_Window_State, win32_state->window_capacity);
	
    // Load cursors
    win32_state->cursors[OS_Cursor_Pointer] = LoadCursor(0, IDC_ARROW);
    win32_state->cursors[OS_Cursor_IBar] = LoadCursor(0, IDC_IBEAM);
    win32_state->cursors[OS_Cursor_Left_Right] = LoadCursor(0, IDC_SIZEWE);
    win32_state->cursors[OS_Cursor_Up_Down] = LoadCursor(0, IDC_SIZENS);
    win32_state->cursors[OS_Cursor_Down_Right] = LoadCursor(0, IDC_SIZENWSE);
    win32_state->cursors[OS_Cursor_Up_Down_Left_Right] = LoadCursor(0, IDC_SIZEALL);
    win32_state->cursors[OS_Cursor_Hand_Point] = LoadCursor(0, IDC_HAND);
    win32_state->cursors[OS_Cursor_Disabled] = LoadCursor(0, IDC_NO);
    win32_state->cursors[OS_Cursor_Loading] = LoadCursor(0, IDC_WAIT);
	
    win32_state->current_cursor = OS_Cursor_Pointer;
    MemoryZeroArray(win32_state->key_down);
    win32_state->current_modifiers = 0;

    SetProcessDPIAware();
    timeBeginPeriod(1);
}

Gfx_Info os_get_gfx_info(void) {
    Gfx_Info info = {0};
    info.double_click_time = (f32)GetDoubleClickTime() / 1000.0f;
    info.caret_blink_time = (f32)GetCaretBlinkTime() / 1000.0f;
    info.default_refresh_rate = 60.0f;
    return info;
}

OS_Handle os_window_open(Gfx_Window_Flags flags, Vec2_s64 size, String title) {
    if (!win32_state || win32_state->window_count >= win32_state->window_capacity) {
        return os_handle_zero();
    }
	
    Win32_Window_State *window_state = &win32_state->windows[win32_state->window_count];
	
    // Convert title to null-terminated string
    Scratch scratch = arena_begin_scratch(0);
    u8     *null_term_title = push_array(scratch.arena, u8, title.size + 1);
    MemoryCopy(null_term_title, title.str, title.size);
    null_term_title[title.size] = 0;
	
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT  window_rect = {0, 0, (LONG)size.x, (LONG)size.y};
    AdjustWindowRect(&window_rect, style, FALSE);
	
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;
	
    // Center window on screen
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    int pos_x = (screen_width - window_width) / 2;
    int pos_y = (screen_height - window_height) / 2;
	
    window_state->hwnd = CreateWindowExA(
										 0,
										 "AppWindowClass",
										 (char *)null_term_title,
										 style,
										 pos_x, pos_y,
										 window_width, window_height,
										 0, 0,
										 win32_state->instance,
										 0);
	
    arena_end_scratch(&scratch);
	
    if (!window_state->hwnd) {
        return os_handle_zero();
    }

    SetWindowLongPtrA(window_state->hwnd, GWLP_USERDATA, (LONG_PTR)window_state);

    HDC hdc = GetDC(window_state->hwnd);
    window_state->dpi_scale = (f32)GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(window_state->hwnd, hdc);
	
    RECT client_rect;
    GetClientRect(window_state->hwnd, &client_rect);
    window_state->size.x = client_rect.right - client_rect.left;
    window_state->size.y = client_rect.bottom - client_rect.top;
    window_state->is_focused = false;
    window_state->is_maximized = false;
    window_state->is_fullscreen = false;
    window_state->is_minimized = false;
    window_state->has_close_event = false;
	
    ShowWindow(window_state->hwnd, SW_SHOW);
    UpdateWindow(window_state->hwnd);
	
    OS_Handle handle = {0};
    handle.v[0] = win32_state->window_count + 1;
    win32_state->window_count++;
	
    return handle;
}

void os_window_close(OS_Handle handle) {
    if (!win32_state || os_handle_match(handle, os_handle_zero())) {
        return;
    }
	
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        DestroyWindow(window_state->hwnd);
        MemoryZeroStruct(window_state);
    }
}

void os_window_set_title(OS_Handle handle, String title) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        Scratch scratch = arena_begin_scratch(0);
        u8     *null_term_title = push_array(scratch.arena, u8, title.size + 1);
        MemoryCopy(null_term_title, title.str, title.size);
        null_term_title[title.size] = 0;
		
        SetWindowTextA(window_state->hwnd, (char *)null_term_title);
		
        arena_end_scratch(&scratch);
    }
}

void os_window_first_paint(OS_Handle handle) {
    // No-op for now
}

Vec2_s32 os_window_get_size(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state) {
        return window_state->size;
    }
    Vec2_s32 zero = {0};
    return zero;
}

void os_window_focus(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        SetForegroundWindow(window_state->hwnd);
        SetFocus(window_state->hwnd);
    }
}

b32 os_window_is_focused(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_focused;
    }
    return false;
}

b32 os_window_is_fullscreen(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_fullscreen;
    }
    return false;
}

void os_window_set_fullscreen(OS_Handle handle, b32 fullscreen) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        if (fullscreen) {
            DWORD style = GetWindowLong(window_state->hwnd, GWL_STYLE);
            SetWindowLong(window_state->hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
			
            MONITORINFO mi = {sizeof(mi)};
            GetMonitorInfo(MonitorFromWindow(window_state->hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
			
            SetWindowPos(window_state->hwnd, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			
            window_state->is_fullscreen = true;
        } else {
            DWORD style = GetWindowLong(window_state->hwnd, GWL_STYLE);
            SetWindowLong(window_state->hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
            SetWindowPos(window_state->hwnd, 0, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
						 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			
            window_state->is_fullscreen = false;
        }
    }
}

b32 os_window_is_maximized(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        return IsZoomed(window_state->hwnd);
    }
    return false;
}

void os_window_set_maximized(OS_Handle handle, b32 maximized) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        ShowWindow(window_state->hwnd, maximized ? SW_MAXIMIZE : SW_RESTORE);
    }
}

b32 os_window_is_minimized(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_minimized;
    }
    return false;
}

void os_window_set_minimized(OS_Handle handle, b32 minimized) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        ShowWindow(window_state->hwnd, minimized ? SW_MINIMIZE : SW_RESTORE);
    }
}

b32 os_window_is_dark_mode(OS_Handle handle) {
    // Check registry for dark mode setting
    HKEY hkey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        if (RegQueryValueExA(hkey, "AppsUseLightTheme", 0, 0, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hkey);
            return value == 0;
        }
        RegCloseKey(hkey);
    }
    return false;
}

b32 os_window_set_dark_mode(OS_Handle handle, b32 dark_mode) {
    // Windows doesn't provide an easy API to set dark mode for individual windows
    return false;
}

void os_window_bring_to_front(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        BringWindowToTop(window_state->hwnd);
    }
}

void os_window_set_monitor(OS_Handle window, OS_Handle monitor) {
    Win32_Window_State *window_state = win32_window_state_from_handle(window);
    if (!window_state || !window_state->hwnd)
        return;
	
    // Get monitor info - monitor handle is stored as HMONITOR in OS_Handle
    HMONITOR hmon = (HMONITOR)(uintptr_t)monitor.v[0];
    if (!hmon) {
        hmon = MonitorFromWindow(window_state->hwnd, MONITOR_DEFAULTTONEAREST);
    }
	
    MONITORINFO mi = {sizeof(MONITORINFO)};
    if (GetMonitorInfo(hmon, &mi)) {
        // Move window to monitor's work area (excludes taskbar)
        RECT *work = &mi.rcWork;
        SetWindowPos(window_state->hwnd, NULL,
                     work->left, work->top,
                     work->right - work->left, work->bottom - work->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void os_window_clear_custom_border(OS_Handle window) {
    Win32_Window_State *window_state = win32_window_state_from_handle(window);
    if (!window_state || !window_state->hwnd)
        return;
	
    // Restore standard window frame
    DWORD style = GetWindowLong(window_state->hwnd, GWL_STYLE);
    style |= WS_CAPTION | WS_THICKFRAME;
    SetWindowLong(window_state->hwnd, GWL_STYLE, style);
	
    // Force frame recalculation
    SetWindowPos(window_state->hwnd, NULL, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void os_window_push_custom_title_bar(OS_Handle window, f32 thickness) {
    Win32_Window_State *window_state = win32_window_state_from_handle(window);
    if (!window_state || !window_state->hwnd)
        return;
	
    // Remove standard caption but keep resize border
    DWORD style = GetWindowLong(window_state->hwnd, GWL_STYLE);
    style &= ~WS_CAPTION;
    style |= WS_THICKFRAME;
    SetWindowLong(window_state->hwnd, GWL_STYLE, style);
	
    // Enable DWM extended frame for shadow effects
    MARGINS margins = {0, 0, (LONG)thickness, 0};
    DwmExtendFrameIntoClientArea(window_state->hwnd, &margins);
	
    // Force frame recalculation
    SetWindowPos(window_state->hwnd, NULL, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

Rng2_f32 os_rect_from_window(OS_Handle handle) {
    Rng2_f32            result = {0};
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        RECT rect;
        GetWindowRect(window_state->hwnd, &rect);
        result.min = V2F32((f32)rect.left, (f32)rect.top);
        result.max = V2F32((f32)rect.right, (f32)rect.bottom);
    }
    return result;
}

Rng2_f32 os_client_from_rect(OS_Handle handle) {
    Rng2_f32            result = {0};
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        RECT rect;
        GetClientRect(window_state->hwnd, &rect);
        result.min = V2F32(0.0f, 0.0f);
        result.max = V2F32((f32)(rect.right - rect.left), (f32)(rect.bottom - rect.top));
    }
    return result;
}

f32 os_dpi_from_window(OS_Handle handle) {
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state) {
        return window_state->dpi_scale;
    }
    return 1.0f;
}

void os_send_wake_up_event(void) {
    if (win32_state && win32_state->window_count > 0) {
        PostMessageA(win32_state->windows[0].hwnd, WM_NULL, 0, 0);
    }
}

OS_Event_List os_get_events(Arena *arena, b32 wait) {
    OS_Event_List result = {0};
	
    if (!win32_state) {
        return result;
    }
	
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);

        switch (msg.message) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN: {
				OS_Key key = os_key_from_win32_vkey(msg.wParam, msg.lParam);
				if (key != OS_Key_Null) {
					OS_Event *event = push_array(arena, OS_Event, 1);
					event->kind = OS_Event_Kind_Press;
					event->key = key;
					event->modifiers = os_modifiers_from_win32_modifiers();
					event->is_repeat = (msg.lParam & (1 << 30)) != 0;

					win32_state->key_down[key] = true;

					SLLQueuePush(result.first, result.last, event);
					result.count++;
				}
			} break;

			case WM_KEYUP:
			case WM_SYSKEYUP: {
				OS_Key key = os_key_from_win32_vkey(msg.wParam, msg.lParam);
				if (key != OS_Key_Null) {
					OS_Event *event = push_array(arena, OS_Event, 1);
					event->kind = OS_Event_Kind_Release;
					event->key = key;
					event->modifiers = os_modifiers_from_win32_modifiers();

					win32_state->key_down[key] = false;

					SLLQueuePush(result.first, result.last, event);
					result.count++;
				}
			} break;

			case WM_CHAR: {
				if (msg.wParam >= 32 && msg.wParam < 127) {
					OS_Event *event = push_array(arena, OS_Event, 1);
					event->kind = OS_Event_Kind_Text;
					event->character = (u32)msg.wParam;

					SLLQueuePush(result.first, result.last, event);
					result.count++;
				}
			} break;

			case WM_LBUTTONDOWN: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Press;
				event->key = OS_Key_LeftMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();
				event->pos.x = (f32)GET_X_LPARAM(msg.lParam);
				event->pos.y = (f32)GET_Y_LPARAM(msg.lParam);

				win32_state->key_down[OS_Key_LeftMouseButton] = true;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_LBUTTONUP: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Release;
				event->key = OS_Key_LeftMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();
				event->pos.x = (f32)GET_X_LPARAM(msg.lParam);
				event->pos.y = (f32)GET_Y_LPARAM(msg.lParam);

				win32_state->key_down[OS_Key_LeftMouseButton] = false;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_RBUTTONDOWN: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Press;
				event->key = OS_Key_RightMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();

				win32_state->key_down[OS_Key_RightMouseButton] = true;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_RBUTTONUP: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Release;
				event->key = OS_Key_RightMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();

				win32_state->key_down[OS_Key_RightMouseButton] = false;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_MBUTTONDOWN: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Press;
				event->key = OS_Key_MiddleMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();

				win32_state->key_down[OS_Key_MiddleMouseButton] = true;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_MBUTTONUP: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Release;
				event->key = OS_Key_MiddleMouseButton;
				event->modifiers = os_modifiers_from_win32_modifiers();

				win32_state->key_down[OS_Key_MiddleMouseButton] = false;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_MOUSEMOVE: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Mouse_Move;
				event->modifiers = os_modifiers_from_win32_modifiers();
				event->pos.x = (f32)GET_X_LPARAM(msg.lParam);
				event->pos.y = (f32)GET_Y_LPARAM(msg.lParam);

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_MOUSEWHEEL: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Scroll;
				event->modifiers = os_modifiers_from_win32_modifiers();
				event->delta.y = (f32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (f32)WHEEL_DELTA;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			case WM_MOUSEHWHEEL: {
				OS_Event *event = push_array(arena, OS_Event, 1);
				event->kind = OS_Event_Kind_Scroll;
				event->modifiers = os_modifiers_from_win32_modifiers();
				event->delta.x = (f32)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (f32)WHEEL_DELTA;

				SLLQueuePush(result.first, result.last, event);
				result.count++;
			} break;

			default:
            DispatchMessageA(&msg);
            break;
        }
    }
	
    // Check for window close events
    for (u64 i = 0; i < win32_state->window_count; i++) {
        Win32_Window_State *window_state = &win32_state->windows[i];
        if (window_state->has_close_event) {
            OS_Event *event = push_array(arena, OS_Event, 1);
            event->kind = OS_Event_Kind_Window_Close;
			
            SLLQueuePush(result.first, result.last, event);
            result.count++;
			
            window_state->has_close_event = false;
        }
    }
	
    return result;
}

OS_Modifiers os_get_modifiers(void) {
    return os_modifiers_from_win32_modifiers();
}

b32 os_key_is_down(OS_Key key) {
    if (!win32_state || key >= OS_Key_COUNT) {
        return false;
    }
    return win32_state->key_down[key];
}

Vec2_f32 os_mouse_from_window(OS_Handle handle) {
    Vec2_f32            result = {0};
    Win32_Window_State *window_state = win32_window_state_from_handle(handle);
    if (window_state && window_state->hwnd) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(window_state->hwnd, &pt);
        result.x = (f32)pt.x;
        result.y = (f32)pt.y;
    }
    return result;
}

void os_graphical_messages(b32 error, String title, String message) {
    Scratch scratch = arena_begin_scratch(0);
	
    u8 *null_term_title = push_array(scratch.arena, u8, title.size + 1);
    MemoryCopy(null_term_title, title.str, title.size);
    null_term_title[title.size] = 0;
	
    u8 *null_term_message = push_array(scratch.arena, u8, message.size + 1);
    MemoryCopy(null_term_message, message.str, message.size);
    null_term_message[message.size] = 0;
	
    MessageBoxA(0, (char *)null_term_message, (char *)null_term_title, error ? MB_ICONERROR : MB_ICONINFORMATION);
	
    arena_end_scratch(&scratch);
}

String os_graphical_pick_file(Arena *arena, String initial_path) {
    String result = {0};
	
    char filename[MAX_PATH] = {0};
	
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = 0;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = 0;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	
    if (GetOpenFileNameA(&ofn)) {
        u64 length = strlen(filename);
        u8 *data = push_array(arena, u8, length);
        MemoryCopy(data, filename, length);
        result = str_range(data, data + length);
    }
	
    return result;
}

Win32_Window_State *win32_window_state_from_handle(OS_Handle handle) {
    if (!win32_state || os_handle_match(handle, os_handle_zero()) || handle.v[0] > win32_state->window_count) {
        return 0;
    }
	
    return &win32_state->windows[handle.v[0] - 1];
}

void win32_set_cursor(OS_Cursor cursor) {
    if (!win32_state || cursor >= OS_Cursor_COUNT) {
        return;
    }
	
    SetCursor(win32_state->cursors[cursor]);
    win32_state->current_cursor = cursor;
}

OS_Key os_key_from_win32_vkey(WPARAM vkey, LPARAM lparam) {
    switch (vkey) {
		case VK_ESCAPE:
        return OS_Key_Esc;
		case VK_F1:
        return OS_Key_F1;
		case VK_F2:
        return OS_Key_F2;
		case VK_F3:
        return OS_Key_F3;
		case VK_F4:
        return OS_Key_F4;
		case VK_F5:
        return OS_Key_F5;
		case VK_F6:
        return OS_Key_F6;
		case VK_F7:
        return OS_Key_F7;
		case VK_F8:
        return OS_Key_F8;
		case VK_F9:
        return OS_Key_F9;
		case VK_F10:
        return OS_Key_F10;
		case VK_F11:
        return OS_Key_F11;
		case VK_F12:
        return OS_Key_F12;
		case VK_OEM_3:
        return OS_Key_Tick;
		case '0':
        return OS_Key_0;
		case '1':
        return OS_Key_1;
		case '2':
        return OS_Key_2;
		case '3':
        return OS_Key_3;
		case '4':
        return OS_Key_4;
		case '5':
        return OS_Key_5;
		case '6':
        return OS_Key_6;
		case '7':
        return OS_Key_7;
		case '8':
        return OS_Key_8;
		case '9':
        return OS_Key_9;
		case VK_OEM_MINUS:
        return OS_Key_Minus;
		case VK_OEM_PLUS:
        return OS_Key_Equal;
		case VK_BACK:
        return OS_Key_Backspace;
		case VK_DELETE:
        return OS_Key_Delete;
		case VK_TAB:
        return OS_Key_Tab;
		case 'A':
        return OS_Key_A;
		case 'B':
        return OS_Key_B;
		case 'C':
        return OS_Key_C;
		case 'D':
        return OS_Key_D;
		case 'E':
        return OS_Key_E;
		case 'F':
        return OS_Key_F;
		case 'G':
        return OS_Key_G;
		case 'H':
        return OS_Key_H;
		case 'I':
        return OS_Key_I;
		case 'J':
        return OS_Key_J;
		case 'K':
        return OS_Key_K;
		case 'L':
        return OS_Key_L;
		case 'M':
        return OS_Key_M;
		case 'N':
        return OS_Key_N;
		case 'O':
        return OS_Key_O;
		case 'P':
        return OS_Key_P;
		case 'Q':
        return OS_Key_Q;
		case 'R':
        return OS_Key_R;
		case 'S':
        return OS_Key_S;
		case 'T':
        return OS_Key_T;
		case 'U':
        return OS_Key_U;
		case 'V':
        return OS_Key_V;
		case 'W':
        return OS_Key_W;
		case 'X':
        return OS_Key_X;
		case 'Y':
        return OS_Key_Y;
		case 'Z':
        return OS_Key_Z;
		case VK_SPACE:
        return OS_Key_Space;
		case VK_RETURN:
        return OS_Key_Return;
		case VK_CONTROL:
        return OS_Key_Ctrl;
		case VK_SHIFT:
        return OS_Key_Shift;
		case VK_MENU:
        return OS_Key_Alt;
		case VK_UP:
        return OS_Key_Up;
		case VK_LEFT:
        return OS_Key_Left;
		case VK_DOWN:
        return OS_Key_Down;
		case VK_RIGHT:
        return OS_Key_Right;
		case VK_PRIOR:
        return OS_Key_PageUp;
		case VK_NEXT:
        return OS_Key_PageDown;
		case VK_HOME:
        return OS_Key_Home;
		case VK_END:
        return OS_Key_End;
		case VK_OEM_2:
        return OS_Key_Slash;
		case VK_OEM_PERIOD:
        return OS_Key_Period;
		case VK_OEM_COMMA:
        return OS_Key_Comma;
		case VK_OEM_7:
        return OS_Key_Quote;
		case VK_OEM_4:
        return OS_Key_LeftBracket;
		case VK_OEM_6:
        return OS_Key_RightBracket;
		case VK_INSERT:
        return OS_Key_Insert;
		case VK_OEM_1:
        return OS_Key_Semicolon;
		case VK_OEM_5:
        return OS_Key_BackSlash;
		case VK_NUMPAD0:
        return OS_Key_Num0;
		case VK_NUMPAD1:
        return OS_Key_Num1;
		case VK_NUMPAD2:
        return OS_Key_Num2;
		case VK_NUMPAD3:
        return OS_Key_Num3;
		case VK_NUMPAD4:
        return OS_Key_Num4;
		case VK_NUMPAD5:
        return OS_Key_Num5;
		case VK_NUMPAD6:
        return OS_Key_Num6;
		case VK_NUMPAD7:
        return OS_Key_Num7;
		case VK_NUMPAD8:
        return OS_Key_Num8;
		case VK_NUMPAD9:
        return OS_Key_Num9;
		case VK_DIVIDE:
        return OS_Key_NumSlash;
		case VK_MULTIPLY:
        return OS_Key_NumStar;
		case VK_SUBTRACT:
        return OS_Key_NumMinus;
		case VK_ADD:
        return OS_Key_NumPlus;
		case VK_DECIMAL:
        return OS_Key_NumPeriod;
		case VK_CAPITAL:
        return OS_Key_CapsLock;
		case VK_NUMLOCK:
        return OS_Key_NumLock;
		case VK_SCROLL:
        return OS_Key_ScrollLock;
		case VK_PAUSE:
        return OS_Key_Pause;
		case VK_APPS:
        return OS_Key_Menu;
		default:
        return OS_Key_Null;
    }
}

OS_Modifiers os_modifiers_from_win32_modifiers(void) {
    OS_Modifiers modifiers = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        modifiers |= OS_Modifier_Ctrl;
    }
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        modifiers |= OS_Modifier_Shift;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        modifiers |= OS_Modifier_Alt;
    }
    return modifiers;
}

#endif // _WIN32
