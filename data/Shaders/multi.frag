#version 140

in vec2 fragTexCoord1;
in vec2 fragTexCoord2;
in vec2 fragTexCoord3;
in vec2 fragTexCoord4;
out vec4 outColor;

uniform vec4 color;
uniform sampler2D textures[4];

void main()
{
	vec4 texel1 = texture(textures[0], fragTexCoord1);
	vec4 texel2 = texture(textures[1], fragTexCoord2);
	vec4 texel3 = texture(textures[2], fragTexCoord3);
	vec4 texel4 = texture(textures[3], fragTexCoord4);

	outColor = texel1;
	outColor.rgb = mix(outColor.rgb, texel2.rgb, texel2.a);
	outColor.rgb = mix(outColor.rgb, texel3.rgb, texel3.a);
	outColor.rgb = mix(outColor.rgb, texel4.rgb, texel4.a);
	outColor = outColor * color;
}