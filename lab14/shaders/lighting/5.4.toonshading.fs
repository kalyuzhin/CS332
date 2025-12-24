#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct Light {
    vec3 position;  
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
   
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 textureColor = texture(material.diffuse, TexCoords).rgb;

    vec3 color;
    if (intensity > 0.95)
        color = textureColor * vec3(1.0, 1.0, 1.0);
    else if (intensity > 0.5)
        color = textureColor * vec3(0.6, 0.6, 0.6);
    else if (intensity > 0.25)
        color = textureColor * vec3(0.4, 0.4, 0.4);
    else
        color = textureColor * vec3(0.2, 0.2, 0.2);

    FragColor = vec4(color, 1.0);
}
