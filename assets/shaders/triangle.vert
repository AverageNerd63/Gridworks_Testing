#version 450

layout(push_constant) uniform PC {
    mat4 vp;
} pc;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_col;

layout(location = 0) out vec3 frag_col;

void main() {
    gl_Position = pc.vp * vec4(in_pos, 1.0);
    frag_col    = in_col;
}