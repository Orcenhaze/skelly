R"(
struct VS_Input
{
	float3 position  : POSITION;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float3 normal    : NORMAL;
	float2 uv        : TEXCOORD;
	float4 color     : COLOR;
};

struct PS_Input
{
	float4   position   : SV_POSITION;
	float3x3 tbn        : TBN;
	float2   uv         : TEXCOORD;
	float4   color      : COLOR;
	float3   pos_world  : POS_WORLD;
};

cbuffer VS_Constants : register(b0)
{
	float4x4 object_to_proj_matrix;
	float4x4 object_to_world_matrix;
}

cbuffer PS_Constants : register(b1)
{
	float3 base_color;
	int    use_normal_map;

	float3 camera_position; // In world space.
	float  metallic;
	
	float3 light_position;  // In world space.
	float  roughness;
	
	float  ambient_occlusion;
}

sampler           sampler0       : register(s0);
Texture2D<float4> albedo_map     : register(t0);
Texture2D<float4> normal_map     : register(t1);
Texture2D<float4> metallic_map   : register(t2);
Texture2D<float4> roughness_map  : register(t3);
Texture2D<float4> ao_map         : register(t4);

PS_Input vs(VS_Input input)
{
	float3 pos_world = mul(object_to_world_matrix, float4(input.position, 1.0f)).xyz;

	// Convert to world space.
	float3 t = normalize(mul(object_to_world_matrix, float4(input.tangent,   0.0f)).xyz);
	float3 b = normalize(mul(object_to_world_matrix, float4(input.bitangent, 0.0f)).xyz);
	float3 n = normalize(mul(object_to_world_matrix, float4(input.normal,    0.0f)).xyz);

	PS_Input output;
	output.position   = mul(object_to_proj_matrix, float4(input.position, 1.0f));
	output.tbn        = float3x3(t, b, n);
	output.uv         = input.uv;
	output.color      = input.color;
	output.pos_world  = pos_world;
	return output;
}

//
// Helper functions (GGX BRDF)
//
static const float PI = 3.14159265f;

float distribution_ggx(float3 N, float3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);

    return num / (PI * denom * denom);
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float4 ps(PS_Input input) : SV_TARGET
{
	// @Note: A good reference for this is "Real Shading in Unreal Engine 4 by Brian Karis, Epic Games".

	float3 albedo_    = base_color.x      < 0.0f? albedo_map   .Sample(sampler0, input.uv).rgb : base_color;
    float3 normal_    = use_normal_map    ==   1? normal_map   .Sample(sampler0, input.uv).rgb : normalize(input.tbn[2]);
	float  metallic_  = metallic          < 0.0f? metallic_map .Sample(sampler0, input.uv).r   : metallic;
	float  roughness_ = roughness         < 0.0f? roughness_map.Sample(sampler0, input.uv).r   : roughness;
	float  ao_        = ambient_occlusion < 0.0f? ao_map       .Sample(sampler0, input.uv).r   : ambient_occlusion;

	// Convert from sRGB to linear space.
	albedo_ = pow(albedo_, 2.2);

	if (use_normal_map == 1) {
		// Convert from tangent space to world space.
		normal_ = normal_ * 2.0 - 1.0;
		normal_ = mul(input.tbn, normal_);
	}

	// Lighting calculations.
    float3 N = normalize(normal_);
    float3 V = normalize(camera_position - input.pos_world);
    float3 L = normalize(light_position - input.pos_world);
    float3 H = normalize(V + L); // Half vector (microfacet normal).

    // Fresnel-Schlick approximation for specular reflection.
	float3 F0 = 0.04; 
	F0        = lerp(F0, albedo_, metallic_);
    float3 F  = F0 + (1.0 - F0) * pow(saturate(1.0 - dot(V, H)), 5.0);

	// Cook-Torrance microfacet BRDF.
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float D     = distribution_ggx(N, H, roughness_);
    float G     = geometry_schlick_ggx(NdotV, roughness_);
    float3 kS   = F;
    float3 kD   = 1.0 - kS;
    kD         *= 1.0 - metallic_;

    float denominator = 4.0 * max(NdotL * NdotV, 0.0) + 0.001;
    float3 specular   = (D * G * F) / denominator;

    // Calculate light radiance.
    float  dist_to_light = length(light_position - input.pos_world);
    float  attenuation   = 1.0 / (dist_to_light * dist_to_light);
    float3 radiance      = attenuation; // Should multiply with light color, but for now assume it's white.

    float3 diffuse = (kD * albedo_) / PI;
    float3 color   = (diffuse + specular) * NdotL * radiance;
    
    float3 ambient = 0.3 * albedo_ * ao_;
    color          = color + ambient;

    // Apply Reinhard tone mapping.
    color = color / (color + 1.0);

    return float4(color, 1.0);
}

)"