#version 460

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 f_pos;
layout(location = 1) in vec4 f_col;
layout(location = 2) in vec2 f_texcoords;
layout (location = 3) in vec2 f_mouse;

layout(location = 0) out vec4 oCol;

void main(){
    oCol = texture(texSampler, f_texcoords) * f_col;
}