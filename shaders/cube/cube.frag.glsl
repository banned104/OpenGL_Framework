#version 330 core

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
    finalColor = vec4(fragTexCoord.x, fragTexCoord.y, 0.5, 1.0);
}
