#version 460

struct ModelViewProj {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout (binding = 0) uniform MVP{
    ModelViewProj mvp[1024];
};

layout (location = 0) in vec3 v_pos;
layout (location = 1) in int  v_trsid;

layout (location = 0) out vec3 f_pos;

void main(){
    gl_Position = 
        vec4(v_pos, 1.0) *
        mvp[v_trsid].model *
        mvp[v_trsid].view *
        mvp[v_trsid].proj
    ;
    f_pos = v_pos;
}