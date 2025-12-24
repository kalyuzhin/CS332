#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct Light {
    //vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light; 

void main()
{
  	vec3 coolColor = vec3(0.0, 0.7, 0.8);
	
	vec3 warmColor = vec3(1.0, 0.6, 0.2);
	
	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);  
    float NdotL = max(dot(norm, lightDir), 0.0);
	
    vec3 diffuseCool = light.diffuse * NdotL * coolColor;
    vec3 diffuseWarm = light.diffuse * NdotL * warmColor;
    
	vec3 kCool = min(coolColor + diffuseCool * texture(material.diffuse, TexCoords).rgb, 1.0);
    vec3 kWarm = min(warmColor + diffuseWarm * texture(material.diffuse, TexCoords).rgb, 1.0);
    
    vec3 kFinal = mix(kCool, kWarm, NdotL);
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;  
        
    vec3 result = kFinal + specular;
	
    FragColor = vec4(min(result, vec3(1.0)), 1.0);
} 