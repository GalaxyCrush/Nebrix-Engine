#version 330 core

in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_Texture;

out vec4 FragColor;

void main() {
    FragColor = texture(u_Texture, v_TexCoord) * v_Color;
}
