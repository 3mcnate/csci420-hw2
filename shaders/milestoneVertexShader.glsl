#version 150

in vec3 position;
in vec4 normal;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

out vec4 col;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);

  // compute the vertex color (into col)
  col = normal;
}

