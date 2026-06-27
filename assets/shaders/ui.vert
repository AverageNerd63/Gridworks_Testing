#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(push_constant) uniform Push { vec2 screen_size; };

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;

void main() {
    vec2 ndc = (aPos / screen_size) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV    = aUV;
    vColor = aColor;
}