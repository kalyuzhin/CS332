#version 330 core
out vec4 FragColor;

// Структуры для источников света 
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform sampler2D texture_diffuse1; // Соответствует diffuseNr = 1 в вашем Mesh::Draw [1]
uniform float shininess = 32.0;

// Прототипы функций
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 1. Вклад глобального направленного света (Луна/Солнце)
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // 2. Вклад прожектора дирижабля (если включен) [2, 4]
    if(spotLight.enabled) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    }

    // 3. Наложение текстуры
    FragColor = vec4(result, 1.0) * texture(texture_diffuse1, TexCoords);
}

// Расчет направленного света [5]
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    return (light.ambient + light.diffuse * diff + light.specular * spec);
}

// Расчет прожектора с мягкими краями и затуханием 
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Диффузное и зеркальное освещение
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Расчет затухания по формуле: 1.0 / (K_c + K_l*d + K_q*d^2) [2]
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // Интенсивность конуса (плавный переход между cutOff и outerCutOff) [3]
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.ambient * attenuation;
    vec3 diffuse = light.diffuse * diff * attenuation * intensity;
    vec3 specular = light.specular * spec * attenuation * intensity;
    
    return (ambient + diffuse + specular);
}