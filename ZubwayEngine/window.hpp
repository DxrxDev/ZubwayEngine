#if !defined(__WINDOW_HPP)
#define      __WINDOW_HPP

#include <vector>
#include <cstdint>

typedef struct VkSurfaceKHR_T *VkSurfaceKHR;

void InitWindowSystem( void );
void *GetWindowSystem( void );

enum class WindowEventType {
    Key, MouseClk, MouseMove
};
enum class MouseButton {
    Left, Middle, Right
};
struct WindowEvent {
    WindowEventType type;
    struct Key {
        char key;
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

    void *GetWindowHandle( void );
private:
    bool keys[UINT8_MAX];
    class WindowImpl;
    WindowImpl *implementation;
};


#endif /* __WINDOW_HPP */