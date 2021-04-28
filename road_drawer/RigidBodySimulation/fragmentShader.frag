/*
	GPU accelerated rigid body simulation with OpenGL and OpenCL.
	Created by: S�ndor Bal�zs - it.sandor.balazs@gmail.com
	AX400
	---
	Fragment shader for the OpenGL visualisation.
*/

// pipeline-b�l bej�v� per-fragment attrib�tumok
in vec3 vs_out_pos;
in vec3 vs_out_normal;
in vec2 vs_out_tex0;

// kimen� �rt�k - a fragment sz�ne
out vec4 fs_out_col;

// uniform v�ltoz�k
// sz�nt�r tulajdons�gok
uniform vec3 eye_pos = vec3(0, 15, 15);

// f�nytulajdons�gok
uniform vec3 light_pos = vec3( 55, 55, 55 );

uniform vec4 La = vec4(0.1f, 0.1f, 0.1f, 1);
uniform vec4 Ld = vec4(0.75f, 0.75f, 0.75f, 1);
uniform vec4 Ls = vec4(1, 1, 1, 1);

// anyagtulajdons�gok
uniform vec4 Ka = vec4(1, 1, 1, 0);
uniform vec4 Kd = vec4(1, 1, 1, 1);
uniform vec4 Ks = vec4(1, 1, 1, 0);
uniform float specular_power = 32;
uniform sampler2D texImage;

void main()
{
	// ambiens sz�n sz�m�t�sa
	vec4 ambient = La * Ka;

	// diff�z sz�n sz�m�t�sa
	vec3 normal = normalize( vs_out_normal );
	vec3 toLight = normalize(light_pos - vs_out_pos);
	float di = clamp( dot( toLight, normal), 0.0f, 1.0f );
	vec4 diffuse = vec4(Ld.rgb*Kd.rgb*di, Kd.a);

	// f�nyfoltk�pz� sz�n
	vec4 specular = vec4(0);

	if ( di > 0 ) {
		vec3 e = normalize( eye_pos - vs_out_pos );
		vec3 r = reflect( -toLight, normal );
		float si = pow( clamp( dot(e, r), 0.0f, 1.0f ), specular_power );
		specular = Ls*Ks*si;
	}

	fs_out_col = (ambient + diffuse + specular ) * texture(texImage, vs_out_tex0.st);
}