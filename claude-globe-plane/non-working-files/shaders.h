#ifndef SHADERS_H
#define SHADERS_H

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 sunPos;
uniform vec3 moonPos;
uniform vec3 sunColor;
uniform vec3 moonColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

// Simple noise function for continent generation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(a, b, f.x) + (c - a) * f.y * (1.0 - f.x) + (d - b) * f.x * f.y;
}

float continentNoise(vec3 pos) {
    // Convert 3D position to 2D coordinates (like lat/long)
    float theta = atan(pos.z, pos.x);
    float phi = asin(pos.y);
    vec2 uv = vec2(theta * 2.0, phi * 3.0);
    
    // Layer multiple noise octaves for more realistic continents
    float n = 0.0;
    n += noise(uv * 3.0) * 0.5;
    n += noise(uv * 6.0) * 0.25;
    n += noise(uv * 12.0) * 0.125;
    
    // Create clearer land/water boundaries
    n = smoothstep(0.35, 0.45, n);
    
    return n;
}

void main() {
    vec3 norm = normalize(Normal);
    
    // Generate land/water based on position
    float landMask = continentNoise(normalize(FragPos));
    
    // Define colors
    vec3 oceanColor = vec3(0.05, 0.2, 0.5);   // Deep blue ocean
    vec3 landColor = vec3(0.15, 0.4, 0.15);   // Green land
    vec3 baseColor = mix(oceanColor, landColor, landMask);
    
    // Add more prominent desert regions
    if (landMask > 0.5) {
        // Create desert bands around certain latitudes (like Earth's desert belts)
        float latitude = abs(normalize(FragPos).y);
        float desertBelt = smoothstep(0.15, 0.25, latitude) * (1.0 - smoothstep(0.35, 0.45, latitude));
        
        // Add noise-based variation
        float variation = noise(normalize(FragPos).xz * 10.0);
        float desertAmount = desertBelt * 0.7 + variation * 0.3;
        
        vec3 desertColor = vec3(0.8, 0.6, 0.3);  // Sandy/orange desert
        vec3 savannaColor = vec3(0.5, 0.5, 0.2); // Yellowish savanna
        
        // Mix between green, savanna, and desert
        if (desertAmount > 0.3) {
            baseColor = mix(baseColor, savannaColor, smoothstep(0.3, 0.5, desertAmount));
        }
        if (desertAmount > 0.5) {
            baseColor = mix(baseColor, desertColor, smoothstep(0.5, 0.8, desertAmount));
        }
    }
    
    // Sun lighting (warm yellow)
    vec3 sunDir = normalize(sunPos - FragPos);
    float sunDiff = max(dot(norm, sunDir), 0.0);
    vec3 sunLight = sunDiff * sunColor * 0.8;
    
    // Moon lighting (cool blue-white)
    vec3 moonDir = normalize(moonPos - FragPos);
    float moonDiff = max(dot(norm, moonDir), 0.0);
    vec3 moonLight = moonDiff * moonColor * 0.3;
    
    // Ambient light (very dark, space-like)
    vec3 ambient = vec3(0.05, 0.05, 0.1);
    
    // Combine all lighting
    vec3 result = (ambient + sunLight + moonLight) * baseColor;
    
    // Add a subtle atmosphere glow at the edges
    vec3 viewDir = normalize(viewPos - FragPos);
    float rim = 1.0 - max(dot(norm, viewDir), 0.0);
    rim = pow(rim, 2.0);
    result += rim * vec3(0.1, 0.2, 0.4) * 0.5;
    
    // Add specular highlights on water
    if (landMask < 0.5) {
        vec3 halfwayDir = normalize(sunDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        result += spec * sunColor * 0.5;
    }
    
    FragColor = vec4(result, 1.0);
}
)";

#endif // SHADERS_H