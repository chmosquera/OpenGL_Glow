#version  450 core
layout(location = 0) in vec4 vertPos;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;


out vec2 fragTex;
//out int pass2;


void main()
{
	//pass2 = secondpass;

	//gl_Position = P * V * M * vertPos;
	gl_Position = vec4(vertPos.xyz, 1.0);
	//gl_Position = vertPos;


	fragTex = vertTex;
}
