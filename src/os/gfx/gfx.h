#pragma once
#include "../../base/base_inc.h"
#include "../os.h"

typedef struct Gfx_Info Gfx_Info;
struct Gfx_Info {
    f32  double_click_time;
    f32  caret_blink_time;
    f32  default_refresh_rate;
    f32  dpi;
    Vec2 screen_size;
};

typedef u32 Gfx_Window_Flags;
enum {
    Gfx_Window_Flag_Custom_Border = (1 << 0),
    Gfx_Window_Flag_Use_Default_Position = (1 << 0),
};

typedef enum OS_Cursor {
    OS_Cursor_Pointer,
    OS_Cursor_IBar,
    OS_Cursor_Left_Right,
    OS_Cursor_Up_Down,
    OS_Cursor_Down_Right,
    OS_Cursor_Up_Down_Left_Right,
    OS_Cursor_Hand_Point,
    OS_Cursor_Disabled,
    OS_Cursor_Loading,
    OS_Cursor_COUNT,
} OS_Cursor;

typedef enum OS_Key {
    OS_Key_Null,
    OS_Key_Esc,
    OS_Key_F1,
    OS_Key_F2,
    OS_Key_F3,
    OS_Key_F4,
    OS_Key_F5,
    OS_Key_F6,
    OS_Key_F7,
    OS_Key_F8,
    OS_Key_F9,
    OS_Key_F10,
    OS_Key_F11,
    OS_Key_F12,
    OS_Key_F13,
    OS_Key_F14,
    OS_Key_F15,
    OS_Key_F16,
    OS_Key_F17,
    OS_Key_F18,
    OS_Key_F19,
    OS_Key_F20,
    OS_Key_F21,
    OS_Key_F22,
    OS_Key_F23,
    OS_Key_F24,
    OS_Key_Tick,
    OS_Key_0,
    OS_Key_1,
    OS_Key_2,
    OS_Key_3,
    OS_Key_4,
    OS_Key_5,
    OS_Key_6,
    OS_Key_7,
    OS_Key_8,
    OS_Key_9,
    OS_Key_Minus,
    OS_Key_Equal,
    OS_Key_Backspace,
    OS_Key_Tab,
    OS_Key_Q,
    OS_Key_W,
    OS_Key_E,
    OS_Key_R,
    OS_Key_T,
    OS_Key_Y,
    OS_Key_U,
    OS_Key_I,
    OS_Key_O,
    OS_Key_P,
    OS_Key_LeftBracket,
    OS_Key_RightBracket,
    OS_Key_BackSlash,
    OS_Key_CapsLock,
    OS_Key_A,
    OS_Key_S,
    OS_Key_D,
    OS_Key_F,
    OS_Key_G,
    OS_Key_H,
    OS_Key_J,
    OS_Key_K,
    OS_Key_L,
    OS_Key_Semicolon,
    OS_Key_Quote,
    OS_Key_Return,
    OS_Key_Shift,
    OS_Key_Z,
    OS_Key_X,
    OS_Key_C,
    OS_Key_V,
    OS_Key_B,
    OS_Key_N,
    OS_Key_M,
    OS_Key_Comma,
    OS_Key_Period,
    OS_Key_Slash,
    OS_Key_Ctrl,
    OS_Key_Alt,
    OS_Key_Space,
    OS_Key_Menu,
    OS_Key_ScrollLock,
    OS_Key_Pause,
    OS_Key_Insert,
    OS_Key_Home,
    OS_Key_PageUp,
    OS_Key_Delete,
    OS_Key_End,
    OS_Key_PageDown,
    OS_Key_Up,
    OS_Key_Left,
    OS_Key_Down,
    OS_Key_Right,
    OS_Key_Ex0,
    OS_Key_Ex1,
    OS_Key_Ex2,
    OS_Key_Ex3,
    OS_Key_Ex4,
    OS_Key_Ex5,
    OS_Key_Ex6,
    OS_Key_Ex7,
    OS_Key_Ex8,
    OS_Key_Ex9,
    OS_Key_Ex10,
    OS_Key_Ex11,
    OS_Key_Ex12,
    OS_Key_Ex13,
    OS_Key_Ex14,
    OS_Key_Ex15,
    OS_Key_Ex16,
    OS_Key_Ex17,
    OS_Key_Ex18,
    OS_Key_Ex19,
    OS_Key_Ex20,
    OS_Key_Ex21,
    OS_Key_Ex22,
    OS_Key_Ex23,
    OS_Key_Ex24,
    OS_Key_Ex25,
    OS_Key_Ex26,
    OS_Key_Ex27,
    OS_Key_Ex28,
    OS_Key_Ex29,
    OS_Key_NumLock,
    OS_Key_NumSlash,
    OS_Key_NumStar,
    OS_Key_NumMinus,
    OS_Key_NumPlus,
    OS_Key_NumPeriod,
    OS_Key_Num0,
    OS_Key_Num1,
    OS_Key_Num2,
    OS_Key_Num3,
    OS_Key_Num4,
    OS_Key_Num5,
    OS_Key_Num6,
    OS_Key_Num7,
    OS_Key_Num8,
    OS_Key_Num9,
    OS_Key_LeftMouseButton,
    OS_Key_MiddleMouseButton,
    OS_Key_RightMouseButton,
    OS_Key_COUNT,
} OS_Key;

C_LINKAGE_BEGIN
extern String os_g_key_display_string_table[143];
extern String os_g_key_cfg_string_table[143];
C_LINKAGE_END

typedef enum OS_Event_Kind {
    OS_Event_Kind_Null,
    OS_Event_Kind_Press,
    OS_Event_Kind_Drag,
    OS_Event_Kind_Release,
    OS_Event_Kind_Mouse_Move,
    OS_Event_Kind_Text,
    OS_Event_Kind_Scroll,
    OS_Event_Kind_Window_Lose_Focus,
    OS_Event_Kind_Window_Close,
    OS_Event_Kind_File_Drop,
    OS_Event_Kind_Wake_Up,
    OS_Event_Kind_COUNT,
} OS_Event_Kind;

typedef u32 OS_Modifiers;
enum {
    OS_Modifier_Ctrl = (1 << 0),
    OS_Modifier_Shift = (1 << 1),
    OS_Modifier_Alt = (1 << 2),
};

typedef struct OS_Event OS_Event;
struct OS_Event {
    OS_Event     *next;
    OS_Event     *prev;
    u64           timestamp_us;
    OS_Event_Kind kind;
    OS_Modifiers  modifiers;
    OS_Key        key;
    b32           is_repeat;
    b32           right_sided;
    u32           character;
    u32           repeat_count;
    Vec2_f32      pos;
    Vec2_f32      delta;
    String_List   strings;
};

typedef struct OS_Event_List OS_Event_List;
struct OS_Event_List {
    u64       count;
    OS_Event *first;
    OS_Event *last;
};

b32 frame(void);

String        os_string_from_event_kind(OS_Event_Kind kind);
String_List   os_string_list_modifiers(Arena *arena, OS_Modifiers flags);
String        os_string_from_modifiers_key(Arena *arena, OS_Modifiers flags);
u32           os_codepoint_from_modifiers_and_key(OS_Modifiers flags, OS_Key key);
void          os_eat_event(OS_Event_List *events, OS_Event *event);
b32           os_key_press(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
b32           os_key_release(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
OS_Event_List os_event_list_copy(Arena *arena, OS_Event_List *dst, OS_Event_List *to_push);
OS_Event     *os_event_list_push_new(Arena *arena, OS_Event_List *events, OS_Event_Kind kind);

void os_gfx_init(void);

OS_Handle os_window_open(Gfx_Window_Flags flags, Vec2_s64 size, String title);
Gfx_Info  os_get_gfx_info(void);
void      os_window_close(OS_Handle window);
void      os_window_set_title(OS_Handle window, String title);
void      os_window_first_paint(OS_Handle window);
Vec2_s32  os_window_get_size(OS_Handle window);
void      os_window_focus(OS_Handle window);
b32       os_window_is_focused(OS_Handle window);
b32       os_window_is_fullscreen(OS_Handle window);
void      os_window_set_fullscreen(OS_Handle window, b32 fullscreen);
b32       os_window_is_maximized(OS_Handle window);
void      os_window_set_maximized(OS_Handle window, b32 maximized);
b32       os_window_is_minimized(OS_Handle window);
void      os_window_set_minimized(OS_Handle window, b32 minimized);
b32       os_window_is_dark_mode(OS_Handle window);
b32       os_window_set_dark_mode(OS_Handle window, b32 dark_mode);
void      os_window_bring_to_front(OS_Handle window);
void      os_window_set_monitor(OS_Handle window, OS_Handle monitor);
void      os_window_clear_custom_border(OS_Handle window);
void      os_window_push_custom_title_bar(OS_Handle, f32 thickness);
Rng2_f32  os_rect_from_window(OS_Handle window);
Rng2_f32  os_client_from_rect(OS_Handle window);
f32       os_dpi_from_window(OS_Handle window);

void          os_send_wake_up_event(void);
OS_Event_List os_get_events(Arena *arena, b32 wait);
OS_Modifiers  os_get_modifiers(void);
b32           os_key_is_down(OS_Key key);
Vec2_f32      os_mouse_from_window(OS_Handle window);

void   os_graphical_messages(b32 error, String title, String message);
String os_graphical_pick_file(Arena *arena, String initial_path);
