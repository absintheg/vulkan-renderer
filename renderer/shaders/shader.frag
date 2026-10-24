#version 450

layout(location = 0) in vec2 uv_coords;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(uv_coords, 0.0, 1.0);
}
