#version 140

in vec2 position;
in vec2 vertTexCoord1;
in vec2 vertTexCoord2;
in vec2 vertTexCoord3;
in vec2 vertTexCoord4;
uniform mat4 MVP;

out vec2 fragTexCoord1;
out vec2 fragTexCoord2;
out vec2 fragTexCoord3;
out vec2 fragTexCoord4;

void main()
{
    gl_Position = MVP * vec4(position, 0.0, 1.0);
    fragTexCoord1 = vertTexCoord1;
    fragTexCoord2 = vertTexCoord2;
    fragTexCoord3 = vertTexCoord3;
    fragTexCoord4 = vertTexCoord4;
}