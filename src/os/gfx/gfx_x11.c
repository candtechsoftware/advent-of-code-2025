#include "../../base/base_inc.h"
#include "gfx.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#undef Font
#ifdef internal
#    undef internal
#endif
#include <X11/XKBlib.h>
#define internal             static
#define GFY_DEFINED_INTERNAL 1
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <limits.h>
#define X11Font Font

// @TODO(Alex): impl these
// String_List   os_string_list_modifiers(Arena *arena, OS_Modifiers flags);
// String        os_string_from_modifiers_key(Arena *arena, OS_Modifiers flags);
// u32           os_codepoint_from_modifiers_and_key(OS_Modifiers flags, OS_Key key);
// void          os_eat_event(OS_Event_List *events, OS_Event *event);
// b32           os_key_press(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
// b32           os_key_release(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
// OS_Event_List os_event_list_copy(Arena *arena, OS_Event_List *dst, OS_Event_List *to_push);
// OS_Event     *os_event_list_push_new(Arena *arena, OS_Event_List *events, OS_Event_Kind kind);
typedef enum X11_Mouse_Button {
    MOUSE_LEFT = 1,
    MOUSE_MIDDLE = 2,
    MOUSE_RIGHT = 3,
    MOUSE_SCROLL_UP = 4,
    MOUSE_SCROLL_DOWN = 5,
    MOUSE_SCROLL_LEFT = 6,
    MOUSE_SCROLL_RIGHT = 7,
    MOUSE_BACK = 8,
    MOUSE_FORWARD = 9
} X11_Mouse_Button;

typedef struct X11_Window_State X11_Window_State;
struct X11_Window_State {
    Window   window;
    Atom     wm_delete_window;
    Atom     wm_state;
    Atom     net_wm_state;
    Atom     net_wm_state_maximized_horz;
    Atom     net_wm_state_maximized_vert;
    Atom     net_wm_state_fullscreen;
    b32      first_paint_done;
    b32      is_focused;
    b32      is_maximized;
    b32      is_dark_mode;
    b32      is_minimized;
    b32      is_fullscreen;
    f32      dpi_scale;
    Vec2_s32 size;
    Vec2_f32 last_cursor_pos;
    //    OS_Repaint_Func *repaint_func;  @TODO(Alex) validate if we need this
};

typedef struct X11_Atoms X11_Atoms;
struct X11_Atoms {
    Atom WM_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
};

typedef struct X11_State X11_State;
struct X11_State {
    Arena            *arena;
    int               screen;
    Display          *display;
    Window            window;
    XIM               input_method;
    XIC               input_context;
    Colormap          colormap;
    Visual           *visual;
    int               visual_depth;
    X11_Window_State *windows;
    u64               window_count;
    u64               window_cap;
    Cursor            cursors[OS_Cursor_COUNT];
    OS_Cursor         current_cursor;
    OS_Event_List     event_list;
    X11_Atoms         atoms;
    OS_Modifiers      current_modifiers;
    b8                key_down[OS_Key_COUNT];
};

X11_State *x11_state = 0;

static OS_Modifiers
os_modifiers_from_x11_state(unsigned int state) {
    OS_Modifiers modifiers = (OS_Modifiers)0;
    if (state & ControlMask)
        modifiers = (OS_Modifiers)(modifiers | OS_Modifier_Ctrl);
    if (state & ShiftMask)
        modifiers = (OS_Modifiers)(modifiers | OS_Modifier_Shift);
    if (state & Mod1Mask)
        modifiers = (OS_Modifiers)(modifiers | OS_Modifier_Alt);
    return modifiers;
}
static OS_Key
os_key_from_x11_button(X11_Mouse_Button btn) {
    switch (btn) {
    case MOUSE_LEFT: {
        return OS_Key_LeftMouseButton;
    } break;
    case MOUSE_MIDDLE: {
        return OS_Key_MiddleMouseButton;
    } break;
    case MOUSE_RIGHT: {
        return OS_Key_RightMouseButton;

    } break;
    case MOUSE_SCROLL_UP:
    case MOUSE_SCROLL_DOWN:
    case MOUSE_SCROLL_LEFT:
    case MOUSE_SCROLL_RIGHT:
    case MOUSE_BACK:
    case MOUSE_FORWARD:
        return OS_Key_Null;
    }

    return OS_Key_Null;
}
internal OS_Key
os_key_from_x11_keysym(KeySym keysym) {
    switch (keysym) {
    case XK_Escape:
        return OS_Key_Esc;
    case XK_F1:
        return OS_Key_F1;
    case XK_F2:
        return OS_Key_F2;
    case XK_F3:
        return OS_Key_F3;
    case XK_F4:
        return OS_Key_F4;
    case XK_F5:
        return OS_Key_F5;
    case XK_F6:
        return OS_Key_F6;
    case XK_F7:
        return OS_Key_F7;
    case XK_F8:
        return OS_Key_F8;
    case XK_F9:
        return OS_Key_F9;
    case XK_F10:
        return OS_Key_F10;
    case XK_F11:
        return OS_Key_F11;
    case XK_F12:
        return OS_Key_F12;
    case XK_grave:
        return OS_Key_Tick;
    case XK_0:
        return OS_Key_0;
    case XK_1:
        return OS_Key_1;
    case XK_2:
        return OS_Key_2;
    case XK_3:
        return OS_Key_3;
    case XK_4:
        return OS_Key_4;
    case XK_5:
        return OS_Key_5;
    case XK_6:
        return OS_Key_6;
    case XK_7:
        return OS_Key_7;
    case XK_8:
        return OS_Key_8;
    case XK_9:
        return OS_Key_9;
    case XK_minus:
        return OS_Key_Minus;
    case XK_equal:
        return OS_Key_Equal;
    case XK_BackSpace:
        return OS_Key_Backspace;
    case XK_Delete:
        return OS_Key_Delete;
    case XK_Tab:
        return OS_Key_Tab;
    case XK_a:
    case XK_A:
        return OS_Key_A;
    case XK_b:
    case XK_B:
        return OS_Key_B;
    case XK_c:
    case XK_C:
        return OS_Key_C;
    case XK_d:
    case XK_D:
        return OS_Key_D;
    case XK_e:
    case XK_E:
        return OS_Key_E;
    case XK_f:
    case XK_F:
        return OS_Key_F;
    case XK_g:
    case XK_G:
        return OS_Key_G;
    case XK_h:
    case XK_H:
        return OS_Key_H;
    case XK_i:
    case XK_I:
        return OS_Key_I;
    case XK_j:
    case XK_J:
        return OS_Key_J;
    case XK_k:
    case XK_K:
        return OS_Key_K;
    case XK_l:
    case XK_L:
        return OS_Key_L;
    case XK_m:
    case XK_M:
        return OS_Key_M;
    case XK_n:
    case XK_N:
        return OS_Key_N;
    case XK_o:
    case XK_O:
        return OS_Key_O;
    case XK_p:
    case XK_P:
        return OS_Key_P;
    case XK_q:
    case XK_Q:
        return OS_Key_Q;
    case XK_r:
    case XK_R:
        return OS_Key_R;
    case XK_s:
    case XK_S:
        return OS_Key_S;
    case XK_t:
    case XK_T:
        return OS_Key_T;
    case XK_u:
    case XK_U:
        return OS_Key_U;
    case XK_v:
    case XK_V:
        return OS_Key_V;
    case XK_w:
    case XK_W:
        return OS_Key_W;
    case XK_x:
    case XK_X:
        return OS_Key_X;
    case XK_y:
    case XK_Y:
        return OS_Key_Y;
    case XK_z:
    case XK_Z:
        return OS_Key_Z;
    case XK_space:
        return OS_Key_Space;
    case XK_Return:
        return OS_Key_Return;
    case XK_Control_L:
    case XK_Control_R:
        return OS_Key_Ctrl;
    case XK_Shift_L:
    case XK_Shift_R:
        return OS_Key_Shift;
    case XK_Alt_L:
    case XK_Alt_R:
        return OS_Key_Alt;
    case XK_Up:
        return OS_Key_Up;
    case XK_Left:
        return OS_Key_Left;
    case XK_Down:
        return OS_Key_Down;
    case XK_Right:
        return OS_Key_Right;
    case XK_Page_Up:
        return OS_Key_PageUp;
    case XK_Page_Down:
        return OS_Key_PageDown;
    case XK_Home:
        return OS_Key_Home;
    case XK_End:
        return OS_Key_End;
    case XK_slash:
        return OS_Key_Slash;
    case XK_period:
        return OS_Key_Period;
    case XK_comma:
        return OS_Key_Comma;
    case XK_apostrophe:
        return OS_Key_Quote;
    case XK_bracketleft:
        return OS_Key_LeftBracket;
    case XK_bracketright:
        return OS_Key_RightBracket;
    case XK_Insert:
        return OS_Key_Insert;
    case XK_semicolon:
        return OS_Key_Semicolon;
    default:
        return OS_Key_Null;
    }
}
void os_gfx_init(void) {
    if (x11_state) {
        return;
    }

    Arena *arena = arena_alloc();

    x11_state = push_array(arena, X11_State, 1);
    x11_state->arena = arena;

    x11_state->display = XOpenDisplay(NULL);
    if (!x11_state->display) {
        log_error("Failed to open x11 display");
        Assert(0);
        return;
    }

    x11_state->screen = DefaultScreen(x11_state->display);
    x11_state->window = RootWindow(x11_state->display, x11_state->screen);
    x11_state->visual = DefaultVisual(x11_state->display, x11_state->screen);
    x11_state->visual_depth = DefaultDepth(x11_state->display, x11_state->screen);
    x11_state->colormap = DefaultColormap(x11_state->display, x11_state->screen);

    x11_state->atoms.WM_PROTOCOLS = XInternAtom(x11_state->display, "WM_PROTOCOLS", False);
    x11_state->atoms.WM_DELETE_WINDOW = XInternAtom(x11_state->display, "WM_DELETE_WINDOW", False);

    x11_state->input_method = XOpenIM(x11_state->display, 0, 0, 0);
    if (x11_state->input_method) {
        x11_state->input_context = XCreateIC(x11_state->input_method,
                                             XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                             XNClientWindow, x11_state->window,
                                             NULL);
    }
    x11_state->window_cap = 16;
    x11_state->windows = push_array(arena, X11_Window_State, x11_state->window_cap);
    x11_state->cursors[OS_Cursor_Pointer] = XCreateFontCursor(x11_state->display, XC_left_ptr);
    x11_state->cursors[OS_Cursor_IBar] = XCreateFontCursor(x11_state->display, XC_xterm);

    x11_state->cursors[OS_Cursor_Hand_Point] = XCreateFontCursor(x11_state->display, XC_hand2);
    x11_state->cursors[OS_Cursor_Left_Right] = XCreateFontCursor(x11_state->display, XC_sb_h_double_arrow);
    x11_state->cursors[OS_Cursor_Up_Down] = XCreateFontCursor(x11_state->display, XC_sb_v_double_arrow);
    x11_state->cursors[OS_Cursor_Up_Down_Left_Right] = XCreateFontCursor(x11_state->display, XC_fleur);
    x11_state->cursors[OS_Cursor_Disabled] = XCreateFontCursor(x11_state->display, XC_pirate);
    x11_state->cursors[OS_Cursor_Loading] = XCreateFontCursor(x11_state->display, XC_watch);

    x11_state->current_cursor = OS_Cursor_Pointer;
    x11_state->current_modifiers = 0;
    MemoryZeroArray(x11_state->key_down);
}

OS_Handle os_window_open(Gfx_Window_Flags flags, Vec2_s64 size, String title) {
    Assert(x11_state && x11_state->window_count < x11_state->window_cap);

    X11_Window_State    *window_state = &x11_state->windows[x11_state->window_count];
    XSetWindowAttributes window_attr = {0};
    window_attr.background_pixel = BlackPixel(x11_state->display, x11_state->screen);
    window_attr.border_pixel = BlackPixel(x11_state->display, x11_state->screen);
    window_attr.colormap = x11_state->colormap;
    window_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                             FocusChangeMask | StructureNotifyMask | EnterWindowMask |
                             LeaveWindowMask;

    window_state->window = XCreateWindow(x11_state->display,
                                         x11_state->window,
                                         0, 0,
                                         (unsigned int)size.x, (unsigned int)size.y,
                                         1,
                                         x11_state->visual_depth,
                                         InputOutput,
                                         x11_state->visual,
                                         CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
                                         &window_attr);

    if (!window_state->window) {
        return os_handle_zero();
    }

    window_state->wm_delete_window = XInternAtom(x11_state->display, "WM_DELETE_WINDOW", False);
    window_state->wm_state = XInternAtom(x11_state->display, "WM_STATE", False);
    window_state->net_wm_state = XInternAtom(x11_state->display, "_NET_WM_STATE", False);
    window_state->net_wm_state_maximized_horz = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    window_state->net_wm_state_maximized_vert = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    window_state->net_wm_state_fullscreen = XInternAtom(x11_state->display, "_NET_WM_STATE_FULLSCREEN", False);

    XSetWMProtocols(x11_state->display, window_state->window, &window_state->wm_delete_window, 1);
    {
        Scratch scratch = arena_begin_scratch(x11_state->arena);
        u8     *null_term_title = push_array(scratch.arena, u8, title.size + 1);
        MemoryCopy(null_term_title, title.str, title.size);
        null_term_title[title.size] = 0;

        XStoreName(x11_state->display, window_state->window, (char *)null_term_title);
        XSetIconName(x11_state->display, window_state->window, (char *)null_term_title);
        arena_end_scratch(&scratch);
    }
    window_state->size = v2s32((s32)size.x, (s32)size.y);
    window_state->is_focused = 0;
    window_state->is_maximized = 0;
    window_state->is_fullscreen = 0;
    window_state->dpi_scale = 1.0f;
    // window_state->repaint_func = 0;

    OS_Handle handle = {0};
    handle.v[0] = x11_state->window_count + 1;
    x11_state->window_count++;
    return handle;
}

internal X11_Window_State *x11_window_state_from_handle(OS_Handle handle) {
    if (!x11_state || os_handle_match(handle, os_handle_zero()) || handle.v[0] > x11_state->window_count) {
        return 0;
    }
    return &x11_state->windows[handle.v[0] - 1];
}

internal X11_Window_State *x11_window_state_from_xid(Window window) {
    if (!x11_state) {
        return 0;
    }

    for (u64 i = 0; i < x11_state->window_count; i++) {
        if (x11_state->windows[i].window == window) {
            return &x11_state->windows[i];
        }
    }

    return 0;
}

internal OS_Event *x11_push_event(Arena *arena, OS_Event_List *list, OS_Event_Kind kind) {
    OS_Event *event = push_array(arena, OS_Event, 1);
    event->kind = kind;

    if (list->last) {
        list->last->next = event;
        event->prev = list->last;
        list->last = event;
    } else {
        list->first = list->last = event;
    }

    list->count++;
    return event;
}

internal OS_Modifiers x11_modifier_for_key(OS_Key key) {
    switch (key) {
    case OS_Key_Ctrl:
        return OS_Modifier_Ctrl;
    case OS_Key_Shift:
        return OS_Modifier_Shift;
    case OS_Key_Alt:
        return OS_Modifier_Alt;
    default:
        return 0;
    }
}

internal OS_Modifiers x11_apply_modifier(OS_Modifiers modifiers, OS_Key key, b32 is_down) {
    OS_Modifiers flag = x11_modifier_for_key(key);
    if (flag) {
        if (is_down) {
            modifiers = (OS_Modifiers)(modifiers | flag);
        } else {
            modifiers = (OS_Modifiers)(modifiers & ~flag);
        }
    }
    return modifiers;
}

internal void x11_update_modifier_state(OS_Key key, b32 is_down) {
    if (!x11_state) {
        return;
    }
    x11_state->current_modifiers = x11_apply_modifier(x11_state->current_modifiers, key, is_down);
}

Gfx_Info os_get_gfx_info(void) {
    Gfx_Info info = {0};
    info.dpi = 96.f; // Default fallback

    if (x11_state && x11_state->display) {
        Display *dpy = x11_state->display;
        int      screen = DefaultScreen(dpy);

        info.screen_size.x = (f32)DisplayWidth(dpy, screen);
        info.screen_size.y = (f32)DisplayHeight(dpy, screen);

        int width_mm = DisplayWidthMM(dpy, screen);
        if (width_mm > 0) {
            f32 dpi_x = (f32)DisplayWidth(dpy, screen) / ((f32)width_mm / 25.4f);
            if (dpi_x > 0.f && dpi_x < 1000.f) { 
                info.dpi = dpi_x;
            }
        }
    }

    return info;
}

void os_window_close(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        if (x11_state->input_context) {
            XUnsetICFocus(x11_state->input_context);
        }
        XDestroyWindow(x11_state->display, window_state->window);
        XFlush(x11_state->display);
        MemoryZeroStruct(window_state);
    }
}

void os_window_set_title(OS_Handle window, String title) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        Scratch scratch = arena_begin_scratch(x11_state->arena);

        u8 *null_term_title = push_array(scratch.arena, u8, title.size + 1);
        MemoryCopy(null_term_title, title.str, title.size);
        null_term_title[title.size] = 0;

        XStoreName(x11_state->display, window_state->window, (char *)null_term_title);
        XSetIconName(x11_state->display, window_state->window, (char *)null_term_title);
        XFlush(x11_state->display);

        arena_end_scratch(&scratch);
    }
}
void os_window_first_paint(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
    }
    window_state->first_paint_done = 1;
    XMapWindow(x11_state->display, window_state->window);

    if (window_state->is_maximized) {
        Atom wm_state = XInternAtom(x11_state->display, "_NET_WM_STATE", False);
        Atom max_horz = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        Atom max_vert = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = window_state->window;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD;
        xev.xclient.data.l[1] = (long)max_horz;
        xev.xclient.data.l[2] = (long)max_vert;
        xev.xclient.data.l[3] = 1;

        XSendEvent(x11_state->display,
                   DefaultRootWindow(x11_state->display),
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &xev);
    }
    XFlush(x11_state->display);
}
void x11_update_window_properties(X11_Window_State *window_state) {
    Atom           actual_type;
    int            actual_format;
    unsigned long  nitems, bytes_after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(x11_state->display, window_state->window,
                           window_state->net_wm_state, 0, LONG_MAX, False,
                           XA_ATOM, &actual_type, &actual_format, &nitems,
                           &bytes_after, &prop) == Success &&
        prop) {
        Atom *atoms = (Atom *)prop;
        window_state->is_maximized = false;
        window_state->is_fullscreen = false;

        for (unsigned long i = 0; i < nitems; i++) {
            if (atoms[i] == window_state->net_wm_state_maximized_horz ||
                atoms[i] == window_state->net_wm_state_maximized_vert) {
                window_state->is_maximized = true;
            }
            if (atoms[i] == window_state->net_wm_state_fullscreen) {
                window_state->is_fullscreen = true;
            }
        }

        XFree(prop);
    }
}

Vec2_s32 os_window_get_size(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (!window_state) {
        return V2S32(0, 0);
    }
    return window_state->size;
}

void os_window_focus(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        Atom   net_active = XInternAtom(x11_state->display, "_NET_ACTIVE_WINDOW", False);
        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = window_state->window;
        xev.xclient.message_type = net_active;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = CurrentTime;

        XSendEvent(x11_state->display,
                   DefaultRootWindow(x11_state->display),
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &xev);
    }
    XFlush(x11_state->display);
}

b32 os_window_is_focused(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        return window_state->is_focused;
    }
    return false;
}
b32 os_window_is_fullscreen(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        return window_state->is_fullscreen;
    }
    return false;
}
void os_window_set_fullscreen(OS_Handle window, b32 fullscreen) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        Atom wm_state = XInternAtom(x11_state->display, "_NET_WM_STATE", False);
        Atom fullscreen = XInternAtom(x11_state->display, "_NET_WM_STATE_FULLSCREEN", False);

        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = window_state->window;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = (long)fullscreen;
        xev.xclient.data.l[2] = 0;
        xev.xclient.data.l[3] = 1; 

        XSendEvent(x11_state->display,
                   DefaultRootWindow(x11_state->display),
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &xev);
    }
    XFlush(x11_state->display);
}

b32 os_window_is_maximized(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        return window_state->is_maximized;
    }
    return false;
}

void os_window_set_maximized(OS_Handle window, b32 maximized) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);

        Atom wm_state = XInternAtom(x11_state->display, "_NET_WM_STATE", False);
        Atom max_horz = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        Atom max_vert = XInternAtom(x11_state->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

        XEvent xev = {0};
        xev.type = ClientMessage;
        xev.xclient.window = window_state->window;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = (long)max_horz;
        xev.xclient.data.l[2] = (long)max_vert;
        xev.xclient.data.l[3] = 1; 

        XSendEvent(x11_state->display,
                   DefaultRootWindow(x11_state->display),
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &xev);
    }
    XFlush(x11_state->display);
}

b32 os_window_is_minimized(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        return window_state->is_minimized;
    }
    return false;
}

void os_window_set_minimized(OS_Handle window, b32 minimized) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        XIconifyWindow(x11_state->display, window_state->window, x11_state->screen);
    }
    XFlush(x11_state->display);
}

b32 os_window_is_dark_mode(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        x11_update_window_properties(window_state);
        return window_state->is_dark_mode;
    }
    return false;
}
b32 os_window_set_dark_mode(OS_Handle window, b32 dark_mode) {
    (void)window;
    (void)dark_mode;
    return false;
}
void os_window_bring_to_front(OS_Handle window);
void os_window_set_monitor(OS_Handle window, OS_Handle monitor);
void os_window_clear_custom_border(OS_Handle window);
void os_window_push_custom_title_bar(OS_Handle, f32 thickness);

Rng2_f32 os_rect_from_window(OS_Handle window);
Rng2_f32 os_client_from_rect(OS_Handle window);
f32 os_dpi_from_window(OS_Handle window) {
    X11_Window_State *window_state = x11_window_state_from_handle(window);
    if (window_state) {
        return window_state->dpi_scale;
    }
    return 1.0f;
}

void os_send_wake_up_event(void);

OS_Modifiers os_get_modifiers(void);
b32          os_key_is_down(OS_Key key);

internal void x11_process_event(Arena *arena, OS_Event_List *events, XEvent *ev) {
    switch (ev->type) {
    case ClientMessage: {
        if (ev->xclient.message_type == x11_state->atoms.WM_PROTOCOLS &&
            (Atom)ev->xclient.data.l[0] == x11_state->atoms.WM_DELETE_WINDOW) {
            x11_push_event(arena, events, OS_Event_Kind_Window_Close);
        }
    } break;

    case MotionNotify: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xmotion.window);
        Vec2_f32          pos = {(f32)ev->xmotion.x, (f32)ev->xmotion.y};
        Vec2_f32          delta = {0};
        if (window_state) {
            delta.x = pos.x - window_state->last_cursor_pos.x;
            delta.y = pos.y - window_state->last_cursor_pos.y;
            window_state->last_cursor_pos = pos;
        }

        OS_Event *event = x11_push_event(arena, events, OS_Event_Kind_Mouse_Move);
        event->pos = pos;
        event->delta = delta;
        event->modifiers = os_modifiers_from_x11_state(ev->xmotion.state);
        event->timestamp_us = (u64)ev->xmotion.time * 1000ULL;
    } break;

    case ButtonPress: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xbutton.window);
        Vec2_f32          pos = {(f32)ev->xbutton.x, (f32)ev->xbutton.y};
        if (window_state) {
            window_state->last_cursor_pos = pos;
        }

        if (ev->xbutton.button == 4 || ev->xbutton.button == 5 || ev->xbutton.button == 6 || ev->xbutton.button == 7) {
            OS_Event *event = x11_push_event(arena, events, OS_Event_Kind_Scroll);
            event->delta.x = (ev->xbutton.button == 6) ? -1.0f : (ev->xbutton.button == 7) ? 1.0f
                                                                                           : 0.0f;
            event->delta.y = (ev->xbutton.button == 4) ? 1.0f : (ev->xbutton.button == 5) ? -1.0f
                                                                                          : 0.0f;
            event->modifiers = os_modifiers_from_x11_state(ev->xbutton.state);
            event->pos = pos;
            event->timestamp_us = (u64)ev->xbutton.time * 1000ULL;
            break;
        }

        OS_Key key = os_key_from_x11_button(ev->xbutton.button);
        if (key != OS_Key_Null) {
            x11_state->key_down[key] = true;
            x11_update_modifier_state(key, true);

            OS_Event *event = x11_push_event(arena, events, OS_Event_Kind_Press);
            event->key = key;
            event->modifiers = os_modifiers_from_x11_state(ev->xbutton.state);
            event->pos = pos;
            event->timestamp_us = (u64)ev->xbutton.time * 1000ULL;
        }
    } break;

    case ButtonRelease: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xbutton.window);
        Vec2_f32          pos = {(f32)ev->xbutton.x, (f32)ev->xbutton.y};
        if (window_state) {
            window_state->last_cursor_pos = pos;
        }

        OS_Key key = os_key_from_x11_button(ev->xbutton.button);
        if (key != OS_Key_Null) {
            x11_state->key_down[key] = false;
            x11_update_modifier_state(key, false);

            OS_Event *event = x11_push_event(arena, events, OS_Event_Kind_Release);
            event->key = key;
            event->modifiers = os_modifiers_from_x11_state(ev->xbutton.state);
            event->pos = pos;
            event->timestamp_us = (u64)ev->xbutton.time * 1000ULL;
        }
    } break;

    case KeyPress: {
        KeySym ks = NoSymbol;
        Status status = 0;
        char   buffer[8] = {0};
        int    len = 0;

        if (x11_state->input_context) {
            len = Xutf8LookupString(x11_state->input_context, &ev->xkey, buffer, sizeof(buffer), &ks, &status);
        } else {
            len = XLookupString(&ev->xkey, buffer, sizeof(buffer), &ks, 0);
        }

        if (ks == NoSymbol) {
            ks = XLookupKeysym(&ev->xkey, 0);
        }

        OS_Key key = os_key_from_x11_keysym(ks);
        if (key != OS_Key_Null) {
            b32 was_down = x11_state->key_down[key];
            x11_state->key_down[key] = true;
            x11_update_modifier_state(key, true);

            OS_Event    *event = x11_push_event(arena, events, OS_Event_Kind_Press);
            OS_Modifiers modifiers = os_modifiers_from_x11_state(ev->xkey.state);
            modifiers = x11_apply_modifier(modifiers, key, true);

            event->key = key;
            event->modifiers = modifiers;
            event->is_repeat = was_down;
            event->timestamp_us = (u64)ev->xkey.time * 1000ULL;
        }

        if (len > 0) {
            String_Decode decode = str_decode_utf8((u8 *)buffer, (u32)len);
            if (decode.codepoint >= 32) {
                OS_Event *text_event = x11_push_event(arena, events, OS_Event_Kind_Text);
                text_event->character = decode.codepoint;
                text_event->timestamp_us = (u64)ev->xkey.time * 1000ULL;
            }
        }
    } break;

    case KeyRelease: {
        KeySym ks = XLookupKeysym(&ev->xkey, 0);
        OS_Key key = os_key_from_x11_keysym(ks);
        if (key != OS_Key_Null) {
            x11_state->key_down[key] = false;
            x11_update_modifier_state(key, false);

            OS_Event    *event = x11_push_event(arena, events, OS_Event_Kind_Release);
            OS_Modifiers modifiers = os_modifiers_from_x11_state(ev->xkey.state);
            modifiers = x11_apply_modifier(modifiers, key, false);

            event->key = key;
            event->modifiers = modifiers;
            event->timestamp_us = (u64)ev->xkey.time * 1000ULL;
        }
    } break;

    case FocusIn: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xfocus.window);
        if (window_state) {
            window_state->is_focused = true;
        }
    } break;

    case FocusOut: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xfocus.window);
        if (window_state) {
            window_state->is_focused = false;
        }
    } break;

    case ConfigureNotify: {
        X11_Window_State *window_state = x11_window_state_from_xid(ev->xconfigure.window);
        if (window_state) {
            window_state->size = v2s32(ev->xconfigure.width, ev->xconfigure.height);
        }
    } break;

    default:
        break;
    }
}

OS_Event_List os_get_events(Arena *arena, b32 wait) {
    OS_Event_List result = {0};

    if (!x11_state || !x11_state->display) {
        return result;
    }

    Display *dpy = x11_state->display;

    XFlush(dpy);

    while (XPending(dpy) > 0) {
        XEvent ev = {0};
        XNextEvent(dpy, &ev);
        x11_process_event(arena, &result, &ev);
    }

    return result;
}

Vec2_f32 os_mouse_from_window(OS_Handle window);

//@TODO(Alex) do I need these
// it seems like x11 doesn't have very good support for this I may just make a general ui version of this stuff
// and then just use that but I will need to research this more
void os_graphical_messages(b32 error, String title, String message) {
}
//@TODO(Alex) do I need these
// it seems like x11 doesn't have very good support for this I may just make a general ui version of this stuff
// and then just use that but I will need to research this more
String os_graphical_pick_file(Arena *arena, String initial_path) {
    String result = {0};
    return result;
}

#if defined(GFY_DEFINED_INTERNAL)
#    undef internal
#    undef GFY_DEFINED_INTERNAL
#endif
