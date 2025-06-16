#version 330 core

layout(location = 0) in vec3 aPos;   // Pozicija vertex-a

uniform mat4 uM; // Model matrica
uniform mat4 uV; // View matrica  
uniform mat4 uP; // Projection matrica

void main()
{
    gl_Position = uP * uV * uM * vec4(aPos, 1.0);
}