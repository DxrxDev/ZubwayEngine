#if !defined(__WINDOW_HPP)
#define      __WINDOW_HPP

#include <vector>
#include <cstdint>

typedef struct VkSurfaceKHR_T *VkSurfaceKHR;

void InitWindowSystem( void );
void *GetWindowSystem( void );

enum class WindowEventType {
    Key
};
struct WindowEvent {
    WindowEventType type;
    struct Key {
        char key;
        bool pressed;
        bool shift, control;
    };
    union {
        Key key;
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