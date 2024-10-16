#version 150

in vec3 position;
in vec4 color;
out vec4 col;

in vec3 left;
in vec3 right;
in vec3 up;
in vec3 down;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform float scale;
uniform float exponent;
uniform int mode;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  vec3 modifiedPosition = position;
  vec4 modifiedColor = color;

  if (mode != 0) {
    modifiedPosition = (position + left + right + up + down) / 5.0;
    modifiedPosition.y = scale * pow(modifiedPosition.y, exponent);
    float colorValue = pow(position.y, exponent);
    modifiedColor = vec4(colorValue, colorValue, colorValue, colorValue);
  }

  gl_Position = projectionMatrix * modelViewMatrix * vec4(modifiedPosition, 1.0f);
  col = modifiedColor;
}

