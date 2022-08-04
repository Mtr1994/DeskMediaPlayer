#version 330 core

attribute vec4 vertexIn;
attribute vec4 textureIn;
varying vec4 textureOut;

void main (void)
{
    gl_Position = vertexIn;
    textureOut = textureIn;
}
