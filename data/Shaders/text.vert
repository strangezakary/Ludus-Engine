#version 150

in vec2 position;
in vec2 vertTexCoord;
uniform mat4 MVP;

out vec2 fragTexCoord;

void main()
{
    gl_Position = MVP * vec4(position, 0.0, 1.0);
    fragTexCoord = vertTexCoord;
}