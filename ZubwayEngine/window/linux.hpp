#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#define XK_MISCELLANY
#include <X11/keysym.h>

#include <vector>
#include <cstring>

xcb_connection_t *xcb_conn;
xcb_screen_t *xcb_screen;

#include "../error.hpp"

#if !defined(__WINDOW_HPP)

enum class WindowEventType {
    Key, MouseClk, MouseMove
};
#define NUMBER_MOUSE_BUTTONS 3
enum class MouseButton {
    Left, Middle, Right
};
struct WindowEvent {
    WindowEventType type;
    struct Key {
        uint32_t key;
        bool pressed;
        bool shift, control;
    };
    struct MouseClk {
        MouseButton mb;
        bool pressed;
    };
    struct MouseMove {
        float x, y;
        float rootx, rooty;
    };
    struct VisChange{
        bool visable;
    };
    union {
        Key       key;
        MouseClk  mc;
        MouseMove mm;
        VisChange vc;
    };
};
class Window{
public:
    enum CreationFlags{
        None = 0x0,
        Resizable = 0x1
    };
    Window(uint32_t width, uint32_t height, const char *title, CreationFlags flag);
    ~Window();
    
    std::vector<WindowEvent> GetEvents( void );
    bool IsRunning( void );
    bool IsPressed( char c );
    bool IsPressed( MouseButton mb );

    void *GetWindowHandle( void );
private:
    bool keys[UINT8_MAX];
    bool mouse[NUMBER_MOUSE_BUTTONS];
    class WindowImpl;
    WindowImpl *implementation;
};

#endif
    
void InitWindowSystem( void ){
    xcb_conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(xcb_conn)){
        Error() << "Couldnt connect to xcb!!";
    }

    xcb_screen = xcb_setup_roots_iterator( xcb_get_setup(xcb_conn) ).data;
}
void *GetWindowSystem( void ){
    return xcb_conn;
}

static xcb_key_symbols_t* symbols;

class Window::WindowImpl{
public:

    WindowImpl(uint32_t width, uint32_t height, const char *title, CreationFlags flag) : running(true){

        xcb_window = xcb_generate_id(xcb_conn);

        uint32_t windowMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t windowMaskValues[] = {
            xcb_screen->black_pixel,
            (
                XCB_EVENT_MASK_EXPOSURE |
                XCB_EVENT_MASK_KEY_PRESS |
                XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_POINTER_MOTION |
                XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_BUTTON_RELEASE |
                XCB_EVENT_MASK_STRUCTURE_NOTIFY
            )
        };

        xcb_create_window(
            xcb_conn,
            XCB_COPY_FROM_PARENT,
            xcb_window,
            xcb_screen->root,
            400, 200,
            width, height,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            xcb_screen->root_visual,
            windowMask, windowMaskValues
        );

        if (!(flag & Window::CreationFlags::Resizable)){
            xcb_size_hints_t hints;
            xcb_icccm_size_hints_set_min_size(&hints, width, height);
            xcb_icccm_size_hints_set_max_size(&hints, width, height);
            xcb_icccm_size_hints_set_size(&hints, 1, width, height);

            xcb_icccm_set_wm_size_hints(xcb_conn, xcb_window, XCB_ATOM_WM_NORMAL_HINTS, &hints);
        }

        xcb_intern_atom_reply_t *wmDeleteReply = xcb_intern_atom_reply(
            xcb_conn,
            xcb_intern_atom(xcb_conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW"),
            NULL
        );
        xcb_intern_atom_reply_t *wmProtocolsReply = xcb_intern_atom_reply(
            xcb_conn,
            xcb_intern_atom(xcb_conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS"),
            NULL
        );
        wmDeleteWin = wmDeleteReply->atom;
        wmProtocols = wmProtocolsReply->atom;


        xcb_change_property(
            xcb_conn, XCB_PROP_MODE_REPLACE, xcb_window,
            wmProtocolsReply->atom, 4, 32, 1, &wmDeleteReply->atom
        );
        free(wmDeleteReply);
        free(wmProtocolsReply);

        xcb_map_window( xcb_conn, xcb_window );
        xcb_flush( xcb_conn );

        std::string appname(title);
        xcb_change_property(
            xcb_conn,
            XCB_PROP_MODE_REPLACE,
            xcb_window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            appname.length(),
            appname.c_str()
        );
        xcb_flush( xcb_conn );

        symbols = xcb_key_symbols_alloc(xcb_conn);
    }

    ~WindowImpl(){
        xcb_key_symbols_free(symbols);
        xcb_destroy_window( xcb_conn, xcb_window );
        xcb_flush( xcb_conn );
    }

    std::vector<WindowEvent> GetEvents( void ){
        std::vector<WindowEvent> ret = std::vector<WindowEvent>();
        xcb_generic_event_t *event;

        while ((event = xcb_poll_for_event(xcb_conn))){
            switch (event->response_type & ~0x80){
                case XCB_DESTROY_NOTIFY:{
                    xcb_destroy_notify_event_t *e =
                        (xcb_destroy_notify_event_t *)event;

                    if (e->window != xcb_window)
                        break;
                    running = false; 
                } break;

                case XCB_CLIENT_MESSAGE:{
                    xcb_client_message_event_t *e =
                        (xcb_client_message_event_t *)event;
                    
                    if (e->window != xcb_window)
                        break;

                    if (e->data.data32[0] == wmDeleteWin)
                        running = false;
                }; break;

                case XCB_KEY_PRESS:{
                    xcb_key_press_event_t e = *(xcb_key_press_event_t *)event;
                    
                    xcb_keysym_t ks;
                    if (e.detail == 0x32)
                        break;
                    
                    ks = xcb_key_symbols_get_keysym(symbols, e.detail, 0);
                    
                    ret.push_back((WindowEvent){
                        WindowEventType::Key,
                        (WindowEvent::Key){
                            ks,
                            true, false, false
                        }
                    });
                } break;

                case XCB_KEY_RELEASE:{
                    xcb_key_release_event_t e = *(xcb_key_release_event_t *)event;
                    
                    xcb_keysym_t ks;
                    if (e.detail == 0x32)
                        break;

                    ks = xcb_key_symbols_get_keysym(symbols, e.detail, 0);
                    
                    ret.push_back((WindowEvent){
                        WindowEventType::Key,
                        (WindowEvent::Key){
                            ks,
                            false, false, false
                        }
                    });
                } break;
                case XCB_BUTTON_PRESS:{
                    xcb_button_press_event_t e = *(xcb_button_press_event_t *)event;
                    xcb_button_t button = e.detail;
                    
                    WindowEvent ev{};
                    ev.type = WindowEventType::MouseClk;
                    MouseButton mb;
                    switch (button){
                        case 1:{mb = MouseButton::Left;} break;
                        case 2:{mb = MouseButton::Middle;} break;
                        case 3:{mb = MouseButton::Right;} break;
                    }
                    ev.mc = (WindowEvent::MouseClk){
                        mb, true
                    };

                    ret.push_back( ev );
                } break;
                case XCB_BUTTON_RELEASE:{
                    xcb_button_release_event_t e = *(xcb_button_release_event_t *)event;
                    xcb_button_t button = e.detail;
                    
                    WindowEvent ev{};
                    ev.type = WindowEventType::MouseClk;
                    MouseButton mb;
                    switch (button){
                        case 1:{mb = MouseButton::Left;} break;
                        case 2:{mb = MouseButton::Middle;} break;
                        case 3:{mb = MouseButton::Right;} break;
                    }
                    ev.mc = (WindowEvent::MouseClk){
                        mb, false
                    };

                    ret.push_back( ev );
                } break;
                case XCB_MOTION_NOTIFY:{
                    xcb_motion_notify_event_t e = *(xcb_motion_notify_event_t*)event;
                    if (e.event != xcb_window) break;
                    WindowEvent ev{};
                    ev.type = WindowEventType::MouseMove,
                    ev.mm = (WindowEvent::MouseMove){
                            .x = (float)e.event_x,
                            .y = (float)e.event_y,
                            .rootx = (float)e.root_x,
                            .rooty = (float)e.root_y
                    };
                    
                    ret.push_back( ev );
                } break;
                default: {
                    std::cout << "Unhandled Window Event: " << (event->response_type & ~0x80) << std::endl;
                } break;
            }
        }

        xcb_flush( xcb_conn );

        return ret;
    }

    bool IsRunning( void ){
        return running;
    }

    void *GetHandle( void ){
        return &xcb_window;
    }

private:
    bool running;

    xcb_window_t  xcb_window;
    xcb_atom_t wmDeleteWin, wmProtocols;

};
