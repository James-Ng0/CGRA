#version 330 core

//Code referenced https://garykeen27.wixsite.com/portfolio/oren-nayar-shading

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;
uniform vec3 lightDirection;
uniform float alpha;

float intensity = 0.8;
// viewspace data (this must match the output of the fragment shader)
in VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
} f_in;

//framebuffer output
out vec4 fb_color;

vec3 camDir = normalize(vec3(0,0,1));

//Code which determines light direction

vec3 lightDir = camDir;
vec3 lightColour = vec3(1,1,1);
void main()
{
    vec3 normal = f_in.normal;
    const float PI = 3.14159;
 
    //vec3 albedo = texture(textureMap, texCoord).rgb;
    vec3 normals = normalize(normal);
    float roughness = 0.4;//texture(roughMap, texCoord).r;
    float AO = 0.5;//texture(AOMap, texCoord).r;  
    float attenuation = 0.8;
    float NdotV = clamp(dot(normals, camDir), 0.0, 1.0);
    float angleVN = acos(NdotV);
    
    vec3 OrenNayar = vec3(0.0);
    vec3 ambientColour = vec3(0.0);
 
    for (int i = 0; i < 1; i++)
    {
        float radius = 50.0;
 
        float NdotL = clamp(dot(normals, lightDir), 0.0, 1.0);
        float angleLN = acos(NdotL);
 
        float alpha = max(angleVN, angleLN);
        float beta = min(angleVN, angleLN);
        float gamma = cos(angleVN - angleLN);
 
        float roughness2 = roughness * roughness;
 
        float A = 1.0 - 0.5 * (roughness2 / (roughness2 + 0.57));
        float B = 0.45 * (roughness2 / (roughness2 + 0.09));
        float C = sin(alpha) * tan(beta);
 
        vec3 diffuse = uColor * (NdotL * (A + ((B * max(0.0, gamma)) * C)));
 
        OrenNayar += diffuse * attenuation;
        ambientColour += uColor * attenuation;
    }
    vec3 ambience = (0.01 * ambientColour) * AO;
 
    vec3 final = (OrenNayar + ambience); 
    final = pow(final, vec3(1.0 / 2.2));
    
    fb_color = vec4(alpha * final + (1-alpha) * final,1) * intensity;
}