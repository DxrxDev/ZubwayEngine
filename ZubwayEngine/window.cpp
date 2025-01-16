#include "window.hpp"
#include "error.hpp"

#include "string"
#include "cstring"

#if defined(__linux__)

#include "window/linux.hpp"

#elif defined( _WIN32 )
    #error "WINDOWS NOT SUPPORTED"
#else
    #error "NO (valid) OPERATING SYSTEM DETECTED"
#endif

Window::Window(uint32_t width, uint32_t height, const char *title, CreationFlags flag){
    implementation = new WindowImpl(width, height, title, flag);
    for (uint32_t i = 0; i < UINT8_MAX; ++i){
        keys[i] = false;
    }
}

Window::~Window(){
    delete implementation;
}

std::vector<WindowEvent> Window::GetEvents( void ){
    std::vector<WindowEvent> es = implementation->GetEvents();

    for (WindowEvent e : es){
        if (e.type == WindowEventType::Key){
            keys[e.key.key] = e.key.pressed;
        }
    }

    return es;
}

bool Window::IsRunning( void ){
    return implementation->IsRunning();
}

bool Window::IsPressed( char c ){
    return keys[c];
}

void *Window::GetWindowHandle( void ){
    return implementation->GetHandle();
}