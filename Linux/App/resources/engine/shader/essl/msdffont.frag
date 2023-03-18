#version 300 es

//	Copyright (c) 2008-2023 Ryo Suzuki.
//	Copyright (c) 2016-2023 OpenSiv3D Project.
//	Licensed under the MIT License.

precision mediump float;

//
//	Textures
//
uniform sampler2D Texture0;

//
//	PSInput
//
in vec4 Color;
in vec2 UV;

//
//	PSOutput
//
layout(location = 0) out vec4 FragColor;

//
//	Constant Buffer
//
layout(std140) uniform PSConstants2D
{
	vec4 g_colorAdd;
	vec4 g_sdfParam;
	vec4 g_sdfOutlineColor;
	vec4 g_sdfShadowColor;
};

//
//	Functions
//
float median(float r, float g, float b)
{
	return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec2 size = vec2(textureSize(Texture0, 0));
	const float pxRange = 4.0;
	vec2 msdfUnit = (pxRange / size);

	vec3 s = texture(Texture0, UV).rgb;
	float d = median(s.r, s.g, s.b);

	float td = (d - 0.5);
	float textAlpha = clamp(td * dot(msdfUnit, 0.5 / fwidth(UV)) + 0.5, 0.0, 1.0);

	vec4 color = vec4(Color.rgb, Color.a * textAlpha);

	FragColor = (color + g_colorAdd);
}
