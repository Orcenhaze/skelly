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
	float3   normal     : NORMAL;
	float3   pos_to_cam : POS_TO_CAM;
	float2   uv         : TEXCOORD;
	float4   color      : COLOR;
};

cbuffer VS_Constants : register(b0)
{
	float4x4 object_to_proj_matrix;
	float4x4 object_to_world_matrix;
	float3   camera_position; // In world space.
}

cbuffer PS_Constants : register(b1)
{
	float3 light_direction; // Directional light.
	
	// Material info.
	float3 base_color;
	int    use_normal_map;
	float  metallic;
	float  roughness;
	float  ambient_occlusion;
}

// @Todo: Sample from textures if possible. Normal mapping requires use of TBN and such.
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
	output.normal     = n;
	output.pos_to_cam = normalize(camera_position - pos_world);
	output.uv         = input.uv;
	output.color      = input.color;
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

float3 fresnel_schlick_r0(float3 albedo)
{
    return (albedo * 0.04 + 0.96);
}

float4 ps(PS_Input input) : SV_TARGET
{
	// @Note: A good reference for this is "Real Shading in Unreal Engine 4 by Brian Karis, Epic Games".

	float3 albedo_    = base_color.x      < 0.0f? albedo_map   .Sample(sampler0, input.uv).rgb : base_color;
    float3 normal_    = use_normal_map    ==   1? normal_map   .Sample(sampler0, input.uv).rgb : normalize(input.normal);
	float  metallic_  = metallic          < 0.0f? metallic_map .Sample(sampler0, input.uv).r   : metallic;
	float  roughness_ = roughness         < 0.0f? roughness_map.Sample(sampler0, input.uv).r   : roughness;
	float  ao_        = ambient_occlusion < 0.0f? ao_map       .Sample(sampler0, input.uv).r   : ambient_occlusion;

	// Lighting calculations.
    float3 N = normalize(normal_);
    float3 V = normalize(input.pos_to_cam);
    float3 L = normalize(light_direction);
    float3 H = normalize(V + L); // Half vector (microfacet normal).

    // Fresnel-Schlick approximation for specular reflection.
    float3 F0 = fresnel_schlick_r0(albedo_);
    float3 F  = F0 + (1.0 - F0) * pow(1.0 - dot(V, H), 5.0);

	// Cook-Torrance microfacet BRDF.
    float NdotL = max(0.0, dot(N, L));
    float NdotV = max(0.0, dot(N, V));
    float D     = distribution_ggx(N, H, roughness_);
    float G     = geometry_schlick_ggx(NdotV, roughness_);
    float3 kS   = F;
    float3 kD   = 1.0 - kS;
    kD         *= 1.0 - metallic_;

    float denominator = 4.0 * max(NdotL * NdotV, 0.0) + 0.001;
    float3 specular   = (D * G * F) / denominator;

    // Final color calculation.
    // @Todo @Incomplete: This assumes we are not using textures.
    //
    float3 diffuse = (kD * albedo_) / PI;
    float3 ambient = diffuse * ao_;
    float3 color   = (diffuse + specular) * NdotL;

    return float4(color + ambient, 1.0);
}

)"