#version 460

struct ModelViewProj {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout (binding = 0) uniform MVP{
    ModelViewProj mvp;
};

layout (location = 0) in vec3 v_pos;

layout (location = 0) out vec3 f_pos;

void main(){
    gl_Position = vec4(v_pos, 1.0) * mvp.model * mvp.view * mvp.proj;
    f_pos = v_pos;
}