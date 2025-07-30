#version 460

layout (binding = 0) uniform MVP{
    mat4 models[1024];
};

layout (push_constant) uniform pushconstantuniform{
    float mousex;
    float mousey;
};

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec4 v_col;
layout (location = 2) in vec2 v_texcoords;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec4 f_col;
layout (location = 2) out vec2 f_texcoords;
layout (location = 3) out vec2 f_mouse;

void main(){
    gl_Position = vec4(
        (v_pos.x/640.0) - 1.0,
        (v_pos.y/360.0) - 1.0,
        0.5,
        1.0
    );
    f_pos = v_pos;
    f_col = v_col;
    f_texcoords = v_texcoords;
    f_mouse = vec2(mousex, mousey);
}