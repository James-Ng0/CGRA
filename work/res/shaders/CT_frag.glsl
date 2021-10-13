#version 330 core
//Code referenced https://web.archive.org/web/20161027225409/http://ruh.li/GraphicsCookTorrance.html

// uniform data
uniform mat4 uProjectionMatrix;
uniform mat4 uModelViewMatrix;
uniform vec3 uColor;
uniform vec3 lightDirection;
float intensity = 1.5;

// viewspace data (this must match the output of the fragment shader)
in VertexData {
	vec3 position;
	vec3 normal;
	vec2 textureCoord;
} f_in;


// framebuffer output
out vec4 fb_color;

void main() {
//Line of code which determines whether to shine light from a set point or from the view position.
    vec3 lightDirection = normalize(-f_in.position);
    vec3 normal = f_in.normal;
	// set important material values
    float roughnessValue = 0.4; // 0 : smooth, 1: rough
    float F0 = 0.8; // fresnel reflectance at normal incidence
    float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
    vec3 lightColor = vec3(1, 0, 0);

	vec3 normal2 = normalize(normal);

	float NdotL = max(dot(normal, lightDirection), 0.0);

	float specular = 0.0;
    if(NdotL > 0.0){
	vec3 eyeDir = normalize(f_in.position * -1);
	// calculate intermediary values
        vec3 halfVector = normalize(lightDirection + eyeDir);
        float NdotH = max(dot(normal, halfVector), 0.0); 
        float NdotV = max(dot(normal, eyeDir), 0.0); // note: this could also be NdotL, which is the same value
        float VdotH = max(dot(eyeDir, halfVector), 0.0);
        float mSquared = roughnessValue * roughnessValue;
		 // geometric attenuation
        float NH2 = 2.0 * NdotH;
        float g1 = (NH2 * NdotV) / VdotH;
        float g2 = (NH2 * NdotL) / VdotH;
        float geoAtt = min(1.0, min(g1, g2));
     
        // roughness (or: microfacet distribution function)
        // beckmann distribution function
        float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
        float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
        float roughness = r1 * exp(r2);
        
        // fresnel
        // Schlick approximation
        float fresnel = pow(1.0 - VdotH, 5.0);
        fresnel *= (1.0 - F0);
        fresnel += F0;
        
       specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * 3.14);
    }

     vec3 finalValue = lightColor * NdotL * (k + specular * (1.0 - k));
    fb_color = vec4(finalValue, 1.0) * intensity;
}