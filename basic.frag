#version 330 core

out vec4 FragColor;  // Finalna boja piksela

uniform vec4 uColor; // Uniform boja

void main()
{
    FragColor = uColor;
}