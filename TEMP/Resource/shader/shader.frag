#version 330 core
layout(location = 0) out vec4 color;

in vec2 textureCoord;

# 采样器变量
uniform sampler2D ourTexture;

void main()
{
    color = texture(ourTexture, textureCoord);
}
