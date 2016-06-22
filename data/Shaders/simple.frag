#version 150

in vec2 fragTexCoord;
out vec4 outColor;

uniform vec4 color;
uniform sampler2D tex;

void main()
{
    outColor = texture(tex, fragTexCoord) * color;
}