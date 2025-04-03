# Zubway Engine
    
A game engine focused on simplicity and performance


# Project Structure

### App folder
Source files for a game im currently working on, or a demo.
basically a sandbox to test new features for the engine.

### ZubwayEngine folder
Engine source files

### Lua build system
Something new im trying out, CBA with CMake and Premake so im making
Dobuild as a side side project.
```sh
USAGE:
  lua build.lua
```


# Dependancies

### General
- Vulkan 1.0
  - debug layer
- Lua 5.4

### Operating system
- linux
  - GCC
  - xcb
  - vulkan-xcb

- everything else
  - unsupported