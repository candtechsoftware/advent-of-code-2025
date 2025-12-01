#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <limits.h>


#include "../os_inc.h"
#include "gfx.h"

//String_List   os_string_list_modifiers(Arena *arena, OS_Modifiers flags);
//String        os_string_from_modifiers_key(Arena *arena, OS_Modifiers flags);
//u32           os_codepoint_from_modifiers_and_key(OS_Modifiers flags, OS_Key key);
//void          os_eat_event(OS_Event_List *events, OS_Event *event);
//b32           os_key_press(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
//b32           os_key_release(OS_Event_List *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key);
//OS_Event_List os_event_list_copy(Arena *arena, OS_Event_List *dst, OS_Event_List *to_push);
//OS_Event     *os_event_list_push_new(Arena *arena, OS_Event_List *events, OS_Event_Kind kind);

typedef struct MacOS_Window_State MacOS_Window_State;
struct MacOS_Window_State {
    NSWindow *window;
    NSView   *view;
    id        delegate;
    b32       is_focused;
    b32       is_maximized;
    b32       is_fullscreen;
    b32       is_minimized;
    b32       has_close_event;
    f32       dpi_scale;
    Vec2_s32  size;
};

typedef struct MacOS_State MacOS_State;
struct MacOS_State {
    NSApplication      *app;
    MacOS_Window_State *windows;
    u64                 window_count;
    u64                 window_capacity;
    NSCursor           *cursors[OS_Cursor_COUNT];
    OS_Cursor           current_cursor;
    Arena              *arena;
    b32                 key_down[OS_Key_COUNT];
    OS_Modifiers        current_modifiers;
};

MacOS_State *macos_state = 0;

OS_Key              os_key_from_macos_keycode(u16 keycode);
OS_Modifiers        os_modifiers_from_macos_flags(NSEventModifierFlags flags);
MacOS_Window_State *macos_window_state_from_handle(OS_Handle handle);
void                macos_set_cursor(OS_Cursor cursor);

@interface WindowDelegate : NSObject <NSWindowDelegate> {
    MacOS_Window_State *window_state;
    OS_Handle           window_handle;
}
- (id)initWithWindowState:(MacOS_Window_State *)state handle:(OS_Handle)handle;
@end

@implementation WindowDelegate
- (id)initWithWindowState:(MacOS_Window_State *)state handle:(OS_Handle)handle {
    self = [super init];
    if (self) {
        window_state = state;
        window_handle = handle;
    }
    return self;
}

- (BOOL)windowShouldClose:(NSWindow *)sender {
    window_state->has_close_event = true;
    return NO;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    window_state->is_focused = true;
}

- (void)windowDidResignKey:(NSNotification *)notification {
    window_state->is_focused = false;
}

- (void)windowDidResize:(NSNotification *)notification {
    NSRect frame = [window_state->view frame];
    window_state->size.x = (s32)frame.size.width;
    window_state->size.y = (s32)frame.size.height;
}

- (void)windowWillEnterFullScreen:(NSNotification *)notification {
    window_state->is_fullscreen = true;
}

- (void)windowDidExitFullScreen:(NSNotification *)notification {
    window_state->is_fullscreen = false;
}

- (void)windowDidMiniaturize:(NSNotification *)notification {
    window_state->is_minimized = true;
}

- (void)windowDidDeminiaturize:(NSNotification *)notification {
    window_state->is_minimized = false;
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification {
    NSWindow *window = notification.object;
    CGFloat newScale = [window.screen backingScaleFactor];
    window_state->dpi_scale = (f32)newScale;
}
@end

@interface ContentView : NSView {
    MacOS_Window_State *window_state;
}
- (id)initWithFrame:(NSRect)frame windowState:(MacOS_Window_State *)state;
@end

@implementation ContentView
- (id)initWithFrame:(NSRect)frame windowState:(MacOS_Window_State *)state {
    self = [super initWithFrame:frame];
    if (self) {
        window_state = state;
    }
    return self;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}
@end

void os_gfx_init(void) {
    if (macos_state) {
        return;
    }

    @autoreleasepool {
        Arena *arena = arena_alloc();
        macos_state = push_array(arena, MacOS_State, 1);
        macos_state->arena = arena;

        NSMenu     *main_menu = [[NSMenu alloc] init];
        NSMenuItem *app_menu_item = [[NSMenuItem alloc] init];

        NSMenu   *app_menu = [[NSMenu alloc] initWithTitle:@"App"];
        NSString *app_name = [[NSProcessInfo processInfo] processName];

        NSMenuItem *about_item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"About %@", app_name]
                                                            action:@selector(orderFrontStandardAboutPanel:)
                                                     keyEquivalent:@""];
        [app_menu addItem:about_item];
        [app_menu addItem:[NSMenuItem separatorItem]];
        NSMenuItem *quit_item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Quit %@", app_name]
                                                           action:@selector(terminate:)
                                                    keyEquivalent:@"q"];
        [app_menu addItem:quit_item];
        [app_menu_item setSubmenu:app_menu];

        [main_menu addItem:app_menu_item];

        macos_state->app = [NSApplication sharedApplication];
        [macos_state->app setMainMenu:main_menu];
        [macos_state->app setActivationPolicy:NSApplicationActivationPolicyRegular];

        macos_state->window_capacity = 16;
        macos_state->windows = push_array(arena, MacOS_Window_State, macos_state->window_capacity);

        macos_state->cursors[OS_Cursor_Pointer] = [NSCursor arrowCursor];
        macos_state->cursors[OS_Cursor_IBar] = [NSCursor IBeamCursor];
        macos_state->cursors[OS_Cursor_Left_Right] = [NSCursor resizeLeftRightCursor];
        macos_state->cursors[OS_Cursor_Up_Down] = [NSCursor resizeUpDownCursor];
        macos_state->cursors[OS_Cursor_Down_Right] = [NSCursor arrowCursor];
        macos_state->cursors[OS_Cursor_Up_Down_Left_Right] = [NSCursor closedHandCursor];
        macos_state->cursors[OS_Cursor_Hand_Point] = [NSCursor pointingHandCursor];
        macos_state->cursors[OS_Cursor_Disabled] = [NSCursor operationNotAllowedCursor];

        macos_state->current_cursor = OS_Cursor_Pointer;
        MemoryZeroArray(macos_state->key_down);
        macos_state->current_modifiers = 0;
    }
}

Gfx_Info
os_get_gfx_info(void) {
    Gfx_Info info = {0};
    info.double_click_time = 0.5f;
    info.caret_blink_time = 0.5f;
    info.default_refresh_rate = 60.0f;

    @autoreleasepool {
        NSScreen *screen = [NSScreen mainScreen];
        if (screen) {
            NSDictionary *description = [screen deviceDescription];
            NSNumber     *refresh = [description objectForKey:@"NSScreenRefreshRate"];
            if (refresh) {
                info.default_refresh_rate = (f32)[refresh doubleValue];
            }
        }
    }

    return info;
}

OS_Handle
os_window_open(Gfx_Window_Flags flags, Vec2_s64 size, String title) {
    if (!macos_state || macos_state->window_count >= macos_state->window_capacity) {
        return os_handle_zero();
    }

    @autoreleasepool {
        MacOS_Window_State *window_state = &macos_state->windows[macos_state->window_count];

        NSRect            frame = NSMakeRect(0, 0, (CGFloat)size.x, (CGFloat)size.y);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

        window_state->window = [[NSWindow alloc] initWithContentRect:frame
                                                           styleMask:style
                                                             backing:NSBackingStoreBuffered
                                                               defer:NO];

        if (!window_state->window) {
            return os_handle_zero();
        }

        window_state->view = [[ContentView alloc] initWithFrame:frame windowState:window_state];
        [window_state->window setContentView:window_state->view];

        OS_Handle handle = {0};
        handle.v[0] = macos_state->window_count + 1;

        WindowDelegate *delegate = [[WindowDelegate alloc] initWithWindowState:window_state handle:handle];
        window_state->delegate = delegate;
        [window_state->window setDelegate:delegate];

        Scratch scratch = arena_begin_scratch(0);
        u8     *null_term_title = push_array(scratch.arena, u8, title.size + 1);
        MemoryCopy(null_term_title, title.str, title.size);
        null_term_title[title.size] = 0;

        NSString *ns_title = [NSString stringWithUTF8String:(char *)null_term_title];
        [window_state->window setTitle:ns_title];

        arena_end_scratch(&scratch);

        window_state->size = V2S32((s32)size.x, (s32)size.y);
        window_state->is_focused = false;
        window_state->is_maximized = false;
        window_state->is_fullscreen = false;
        window_state->is_minimized = false;
        window_state->has_close_event = false;
        window_state->dpi_scale = (f32)[[window_state->window screen] backingScaleFactor];

        [window_state->window center];
        [window_state->window makeKeyAndOrderFront:nil];
        [window_state->window setAcceptsMouseMovedEvents:YES];
        [macos_state->app activateIgnoringOtherApps:YES];

        macos_state->window_count++;

        return handle;
    }
}

void os_window_close(OS_Handle handle) {
    if (!macos_state || os_handle_match(handle, os_handle_zero())) {
        return;
    }

    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state) {
            if (!window_state->window && !window_state->view) {
                return;
            }

            if (window_state->window) {
                [window_state->window setDelegate:nil];
            }

            if (window_state->window) {
                [window_state->window orderOut:nil];
                [window_state->window close];
            }

            window_state->window = nil;
            window_state->view = nil;
            window_state->delegate = nil;

            MemoryZeroStruct(window_state);
        }
    }
}

void os_window_set_title(OS_Handle handle, String title) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            Scratch scratch = arena_begin_scratch(0);
            u8     *null_term_title = push_array(scratch.arena, u8, title.size + 1);
            MemoryCopy(null_term_title, title.str, title.size);
            null_term_title[title.size] = 0;

            NSString *ns_title = [NSString stringWithUTF8String:(char *)null_term_title];
            [window_state->window setTitle:ns_title];

            arena_end_scratch(&scratch);
        }
    }
}

void os_window_first_paint(OS_Handle handle) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            [window_state->window orderFront:nil];
        }
    }
}

void os_window_focus(OS_Handle handle) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            [window_state->window makeKeyAndOrderFront:nil];
        }
    }
}

b32 os_window_is_focused(OS_Handle handle) {
    MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_focused;
    }
    return false;
}

b32 os_window_is_fullscreen(OS_Handle handle) {
    MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_fullscreen;
    }
    return false;
}

void os_window_set_fullscreen(OS_Handle handle, b32 fullscreen) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            b32 is_fullscreen = ([window_state->window styleMask] & NSWindowStyleMaskFullScreen) != 0;
            if (fullscreen != is_fullscreen) {
                [window_state->window toggleFullScreen:nil];
            }
        }
    }
}

b32 os_window_is_maximized(OS_Handle handle) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            return [window_state->window isZoomed];
        }
    }
    return false;
}

void os_window_set_maximized(OS_Handle handle, b32 maximized) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            b32 is_zoomed = [window_state->window isZoomed];
            if (maximized != is_zoomed) {
                [window_state->window zoom:nil];
            }
        }
    }
}

b32 os_window_is_minimized(OS_Handle handle) {
    MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
    if (window_state) {
        return window_state->is_minimized;
    }
    return false;
}

b32 os_window_is_dark_mode(OS_Handle handle) {
    @autoreleasepool {
        NSString *osxMode = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleInterfaceStyle"];
        return [osxMode isEqualToString:@"Dark"];
    }
}

b32 os_window_set_dark_mode(OS_Handle handle, b32 dark_mode) {
    return false;
}

void os_window_bring_to_front(OS_Handle handle) {
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            [window_state->window orderFront:nil];
        }
    }
}

void os_window_set_monitor(OS_Handle window, OS_Handle monitor) {
}

void os_window_clear_custom_border(OS_Handle window) {
}

void os_window_push_custom_title_bar(OS_Handle window, f32 thickness) {
}

Rng2_f32
os_rect_from_window(OS_Handle handle) {
    Rng2_f32 result = {0};
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            NSRect    frame = [window_state->window frame];
            NSScreen *screen = [window_state->window screen];
            CGFloat   screenHeight = [screen frame].size.height;

            result.min = V2F32((f32)frame.origin.x, (f32)(screenHeight - frame.origin.y - frame.size.height));
            result.max = V2F32((f32)(frame.origin.x + frame.size.width), (f32)(screenHeight - frame.origin.y));
        }
    }
    return result;
}

Rng2_f32
os_client_from_rect(OS_Handle handle) {
    Rng2_f32 result = {0};
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->view) {
            NSRect frame = [window_state->view frame];

            result.min = V2F32(0.0f, 0.0f);
            result.max = V2F32((f32)frame.size.width, (f32)frame.size.height);
        }
    }
    return result;
}

f32 os_dpi_from_window(OS_Handle handle) {
    f32 result = 1.0f;
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            NSScreen *screen = [window_state->window screen];
            if (screen) {
                result = (f32)[screen backingScaleFactor];
            }
        }
    }
    return result;
}

void os_send_wake_up_event(void) {
    @autoreleasepool {
        NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSMakePoint(0, 0)
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:event atStart:YES];
    }
}

OS_Event_List
os_get_events(Arena *arena, b32 wait) {
    OS_Event_List result = {0};

    if (!macos_state) {
        return result;
    }

    @autoreleasepool {
        NSEvent *event;
        NSDate  *until = wait ? [NSDate distantFuture] : [NSDate distantPast];

        while ((event = [macos_state->app nextEventMatchingMask:NSEventMaskAny
                                                      untilDate:until
                                                         inMode:NSDefaultRunLoopMode
                                                        dequeue:YES])) {
            [macos_state->app sendEvent:event];

            NSWindow *ns_window = [event window];
            OS_Handle window_handle = os_handle_zero();

            for (u64 i = 0; i < macos_state->window_count; i++) {
                if (macos_state->windows[i].window == ns_window) {
                    window_handle.v[0] = i + 1;
                    break;
                }
            }

            switch ([event type]) {
            case NSEventTypeKeyDown: {
                OS_Key key = os_key_from_macos_keycode([event keyCode]);
                if (key != OS_Key_Null) {
                    OS_Event *os_event = push_array(arena, OS_Event, 1);
                    os_event->kind = OS_Event_Kind_Press;
                    os_event->key = key;
                    os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                    os_event->is_repeat = [event isARepeat];
                    os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                    macos_state->key_down[key] = true;

                    SLLQueuePush(result.first, result.last, os_event);
                    result.count++;

                    NSString *characters = [event characters];
                    if ([characters length] > 0) {
                        unichar character = [characters characterAtIndex:0];
                        if (character >= 32 && character < 127) {
                            OS_Event *text_event = push_array(arena, OS_Event, 1);
                            text_event->kind = OS_Event_Kind_Text;
                            text_event->character = character;
                            text_event->timestamp_us = os_event->timestamp_us;

                            SLLQueuePush(result.first, result.last, text_event);
                            result.count++;
                        }
                    }
                }
            } break;

            case NSEventTypeKeyUp: {
                OS_Key key = os_key_from_macos_keycode([event keyCode]);
                if (key != OS_Key_Null) {
                    OS_Event *os_event = push_array(arena, OS_Event, 1);
                    os_event->kind = OS_Event_Kind_Release;
                    os_event->key = key;
                    os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                    os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                    macos_state->key_down[key] = false;

                    SLLQueuePush(result.first, result.last, os_event);
                    result.count++;
                }
            } break;

            case NSEventTypeLeftMouseDown: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Press;
                os_event->key = OS_Key_LeftMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                MacOS_Window_State *window_state = macos_window_state_from_handle(window_handle);
                if (window_state && window_state->window) {
                    NSPoint mouse_loc = [event locationInWindow];
                    NSRect  frame = [[window_state->window contentView] frame];
                    os_event->pos.x = (f32)mouse_loc.x;
                    os_event->pos.y = (f32)(frame.size.height - mouse_loc.y);
                }

                macos_state->key_down[OS_Key_LeftMouseButton] = true;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeLeftMouseUp: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Release;
                os_event->key = OS_Key_LeftMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                MacOS_Window_State *window_state = macos_window_state_from_handle(window_handle);
                if (window_state && window_state->window) {
                    NSPoint mouse_loc = [event locationInWindow];
                    NSRect  frame = [[window_state->window contentView] frame];
                    os_event->pos.x = (f32)mouse_loc.x;
                    os_event->pos.y = (f32)(frame.size.height - mouse_loc.y);
                }

                macos_state->key_down[OS_Key_LeftMouseButton] = false;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeRightMouseDown: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Press;
                os_event->key = OS_Key_RightMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                macos_state->key_down[OS_Key_RightMouseButton] = true;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeRightMouseUp: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Release;
                os_event->key = OS_Key_RightMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                macos_state->key_down[OS_Key_RightMouseButton] = false;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeOtherMouseDown: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Press;
                os_event->key = OS_Key_MiddleMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                macos_state->key_down[OS_Key_MiddleMouseButton] = true;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeOtherMouseUp: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Release;
                os_event->key = OS_Key_MiddleMouseButton;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                macos_state->key_down[OS_Key_MiddleMouseButton] = false;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Mouse_Move;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                MacOS_Window_State *window_state = macos_window_state_from_handle(window_handle);
                if (window_state && window_state->window) {
                    NSPoint mouse_loc = [event locationInWindow];
                    NSRect  frame = [[window_state->window contentView] frame];
                    os_event->pos.x = (f32)mouse_loc.x;
                    os_event->pos.y = (f32)(frame.size.height - mouse_loc.y);
                }

                os_event->delta.x = (f32)[event deltaX];
                os_event->delta.y = -(f32)[event deltaY];

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeScrollWheel: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Scroll;
                os_event->modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                CGFloat deltaX = [event scrollingDeltaX];
                CGFloat deltaY = [event scrollingDeltaY];

                if ([event hasPreciseScrollingDeltas]) {
                    deltaX *= 0.1;
                    deltaY *= 0.1;
                }

                os_event->delta.x = (f32)deltaX;
                os_event->delta.y = (f32)deltaY;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            case NSEventTypeFlagsChanged: {
                OS_Modifiers new_modifiers = os_modifiers_from_macos_flags([event modifierFlags]);
                macos_state->current_modifiers = new_modifiers;
            } break;

            case NSEventTypeApplicationDefined: {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Wake_Up;
                os_event->timestamp_us = (u64)([event timestamp] * 1000000.0);

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;
            } break;

            default:
                break;
            }

            until = [NSDate distantPast];
        }

        for (u64 i = 0; i < macos_state->window_count; i++) {
            MacOS_Window_State *window_state = &macos_state->windows[i];
            if (window_state->has_close_event) {
                OS_Event *os_event = push_array(arena, OS_Event, 1);
                os_event->kind = OS_Event_Kind_Window_Close;
                os_event->timestamp_us = 0;

                SLLQueuePush(result.first, result.last, os_event);
                result.count++;

                window_state->has_close_event = false;
            }
        }
    }

    return result;
}

OS_Modifiers
os_get_modifiers(void) {
    if (!macos_state) {
        return 0;
    }
    return macos_state->current_modifiers;
}

b32 os_key_is_down(OS_Key key) {
    if (!macos_state || key >= OS_Key_COUNT) {
        return false;
    }
    return macos_state->key_down[key];
}

Vec2_f32
os_mouse_from_window(OS_Handle handle) {
    Vec2_f32 result = {0};
    @autoreleasepool {
        MacOS_Window_State *window_state = macos_window_state_from_handle(handle);
        if (window_state && window_state->window) {
            NSPoint mouse_loc = [window_state->window mouseLocationOutsideOfEventStream];
            NSRect  frame = [[window_state->window contentView] frame];
            result.x = (f32)mouse_loc.x;
            result.y = (f32)(frame.size.height - mouse_loc.y);
        }
    }
    return result;
}

void os_graphical_messages(b32 error, String title, String message) {
    @autoreleasepool {
        Scratch scratch = arena_begin_scratch(0);

        u8 *null_term_title = push_array(scratch.arena, u8, title.size + 1);
        MemoryCopy(null_term_title, title.str, title.size);
        null_term_title[title.size] = 0;

        u8 *null_term_message = push_array(scratch.arena, u8, message.size + 1);
        MemoryCopy(null_term_message, message.str, message.size);
        null_term_message[message.size] = 0;

        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:[NSString stringWithUTF8String:(char *)null_term_title]];
        [alert setInformativeText:[NSString stringWithUTF8String:(char *)null_term_message]];
        [alert setAlertStyle:error ? NSAlertStyleCritical : NSAlertStyleInformational];
        [alert addButtonWithTitle:@"OK"];
        [alert runModal];

        arena_end_scratch(&scratch);
    }
}

String
os_graphical_pick_file(Arena *arena, String initial_path) {
    String result = {0};
    @autoreleasepool {
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL      *url = [[panel URLs] objectAtIndex:0];
            NSString   *path = [url path];
            const char *path_str = [path UTF8String];
            u64         length = strlen(path_str);

            u8 *data = push_array(arena, u8, length);
            MemoryCopy(data, (void *)path_str, length);
            result = str_range(data, data + (u64)length);
        }
    }
    return result;
}

MacOS_Window_State *
macos_window_state_from_handle(OS_Handle handle) {
    if (!macos_state || os_handle_match(handle, os_handle_zero()) || handle.v[0] > macos_state->window_count) {
        return 0;
    }

    return &macos_state->windows[handle.v[0] - 1];
}

void macos_set_cursor(OS_Cursor cursor) {
    if (!macos_state || cursor >= OS_Cursor_COUNT) {
        return;
    }

    @autoreleasepool {
        [macos_state->cursors[cursor] set];
        macos_state->current_cursor = cursor;
    }
}

OS_Key
os_key_from_macos_keycode(u16 keycode) {
    switch (keycode) {
    case 53:
        return OS_Key_Esc;
    case 122:
        return OS_Key_F1;
    case 120:
        return OS_Key_F2;
    case 99:
        return OS_Key_F3;
    case 118:
        return OS_Key_F4;
    case 96:
        return OS_Key_F5;
    case 97:
        return OS_Key_F6;
    case 98:
        return OS_Key_F7;
    case 100:
        return OS_Key_F8;
    case 101:
        return OS_Key_F9;
    case 109:
        return OS_Key_F10;
    case 103:
        return OS_Key_F11;
    case 111:
        return OS_Key_F12;
    case 50:
        return OS_Key_Tick;
    case 29:
        return OS_Key_0;
    case 18:
        return OS_Key_1;
    case 19:
        return OS_Key_2;
    case 20:
        return OS_Key_3;
    case 21:
        return OS_Key_4;
    case 23:
        return OS_Key_5;
    case 22:
        return OS_Key_6;
    case 26:
        return OS_Key_7;
    case 28:
        return OS_Key_8;
    case 25:
        return OS_Key_9;
    case 27:
        return OS_Key_Minus;
    case 24:
        return OS_Key_Equal;
    case 51:
        return OS_Key_Backspace;
    case 117:
        return OS_Key_Delete;
    case 48:
        return OS_Key_Tab;
    case 0:
        return OS_Key_A;
    case 11:
        return OS_Key_B;
    case 8:
        return OS_Key_C;
    case 2:
        return OS_Key_D;
    case 14:
        return OS_Key_E;
    case 3:
        return OS_Key_F;
    case 5:
        return OS_Key_G;
    case 4:
        return OS_Key_H;
    case 34:
        return OS_Key_I;
    case 38:
        return OS_Key_J;
    case 40:
        return OS_Key_K;
    case 37:
        return OS_Key_L;
    case 46:
        return OS_Key_M;
    case 45:
        return OS_Key_N;
    case 31:
        return OS_Key_O;
    case 35:
        return OS_Key_P;
    case 12:
        return OS_Key_Q;
    case 15:
        return OS_Key_R;
    case 1:
        return OS_Key_S;
    case 17:
        return OS_Key_T;
    case 32:
        return OS_Key_U;
    case 9:
        return OS_Key_V;
    case 13:
        return OS_Key_W;
    case 7:
        return OS_Key_X;
    case 16:
        return OS_Key_Y;
    case 6:
        return OS_Key_Z;
    case 49:
        return OS_Key_Space;
    case 36:
        return OS_Key_Return;
    case 59:
    case 62:
        return OS_Key_Ctrl;
    case 56:
    case 60:
        return OS_Key_Shift;
    case 58:
    case 61:
        return OS_Key_Alt;
    case 126:
        return OS_Key_Up;
    case 123:
        return OS_Key_Left;
    case 125:
        return OS_Key_Down;
    case 124:
        return OS_Key_Right;
    case 116:
        return OS_Key_PageUp;
    case 121:
        return OS_Key_PageDown;
    case 115:
        return OS_Key_Home;
    case 119:
        return OS_Key_End;
    case 44:
        return OS_Key_Slash;
    case 47:
        return OS_Key_Period;
    case 43:
        return OS_Key_Comma;
    case 39:
        return OS_Key_Quote;
    case 33:
        return OS_Key_LeftBracket;
    case 30:
        return OS_Key_RightBracket;
    case 114:
        return OS_Key_Insert;
    case 41:
        return OS_Key_Semicolon;
    default:
        return OS_Key_Null;
    }
}

OS_Modifiers
os_modifiers_from_macos_flags(NSEventModifierFlags flags) {
    OS_Modifiers modifiers = 0;
    if (flags & NSEventModifierFlagControl)
        modifiers |= OS_Modifier_Ctrl;
    if (flags & NSEventModifierFlagShift)
        modifiers |= OS_Modifier_Shift;
    if (flags & NSEventModifierFlagOption)
        modifiers |= OS_Modifier_Alt;
    return modifiers;
}
