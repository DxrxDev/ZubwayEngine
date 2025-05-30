#version 460

layout (binding = 0) uniform MVP{
    mat4 models[1024];
};

layout (push_constant) uniform pushconstantuniform{
    mat4 viewproj;
};

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec2 v_texcoords;
layout (location = 2) in int  v_trsid;

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec2 f_texcoords;

void main(){
    gl_Position = 
        vec4(v_pos, 1.0) *
        models[v_trsid] *
        viewproj
    ;
    f_pos = v_pos;
    f_texcoords = v_texcoords;
}