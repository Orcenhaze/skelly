R"(

struct VS_Input
{
	float3 position  : POSITION;
	float3 tangent   : TANGENT;
	float3 bitangent : BITANGENT;
	float3 normal    : NORMAL;
	float2 uv        : TEXCOORD;
	float4 color     : COLOR;

	// V4 because MAX_JOINTS_PER_VERTEX is 4.
    int4   joint_ids : JOINT_IDS;
    float4 weights   : WEIGHTS;
};

struct PS_Input
{
	float4   position   : SV_POSITION;
	float3x3 tbn        : TBN;
	float2   uv         : TEXCOORD;
	float4   color      : COLOR;
	float3   pos_world  : POS_WORLD;
};

#define MAX_JOINTS_PER_VERTEX 4
#define MAX_JOINTS 65
#define VSConstantsFlags_SHOULD_SKIN 0x1
cbuffer VS_Constants : register(b0)
{
	float4x4 skinning_matrices[MAX_JOINTS];
	float4x4 object_to_proj_matrix;
	float4x4 object_to_world_matrix;
	uint flags;
}

static const float PI = 3.14159265f;

// If you modify max lights allowed, don't forget to also change it in (d3d11_orh.h)
static const int MAX_POINT_LIGHTS = 5;
static const int MAX_DIR_LIGHTS   = 2;
cbuffer PS_Constants : register(b1)
{
	struct
	{
		float3 position;  // In world space
		float  intensity; // Unitless
		float3 color;
		float  range;	  // In world space units
	} point_lights[MAX_POINT_LIGHTS];

	struct
	{
		float3 direction; // In world space
		float  intensity; // Unitless
		float3 color;
		float  indirect_lighting_intensity; // Unitless
	} dir_lights[MAX_DIR_LIGHTS];

	float3 camera_position; // In world space
	int    use_normal_map;

	float3 base_color;
	float  metallic;
	float  roughness;
	float  ambient_occlusion;

	int num_point_lights;
	int num_dir_lights;
}

sampler           sampler0       : register(s0);
Texture2D<float4> albedo_map     : register(t0);
Texture2D<float4> normal_map     : register(t1);
Texture2D<float4> metallic_map   : register(t2);
Texture2D<float4> roughness_map  : register(t3);
Texture2D<float4> ao_map         : register(t4);

PS_Input vs(VS_Input input)
{
	//
	// Skin the input vertex and normal if required.
	//
	float4 pos;
	float4 nor;
	if (flags & VSConstantsFlags_SHOULD_SKIN) {
		pos = float4(0,0,0,0);
		nor = float4(0,0,0,0);
		
		for (int piece_index = 0; piece_index < MAX_JOINTS_PER_VERTEX; piece_index++) {
			int joint_id = input.joint_ids[piece_index];
			if (joint_id == -1) continue;
			if (joint_id >= MAX_JOINTS) {
				pos = float4(input.position, 1.0f);
				nor = float4(input.normal,   0.0f);
				break;
			}
			
			float4x4 m = skinning_matrices[joint_id];
			pos       += mul(m, float4(input.position, 1.0f)) * input.weights[piece_index];
			nor       += mul(m, float4(input.normal,   0.0f)) * input.weights[piece_index];
		}

		if (pos.w)
			pos.xyz /= pos.w;

		pos.w = 1.0f;
		nor.w = 0.0f;
	} else {
		pos = float4(input.position, 1.0f);
		nor = float4(input.normal,   0.0f);
	}

	//
	// Convert to world space.
	//
	float3 pos_world = mul(object_to_world_matrix, pos).xyz;

	float3 t = normalize(mul(object_to_world_matrix, float4(input.tangent,   0.0f)).xyz);
	float3 b = normalize(mul(object_to_world_matrix, float4(input.bitangent, 0.0f)).xyz);
	float3 n = normalize(mul(object_to_world_matrix, nor                          ).xyz);

	PS_Input output;
	output.position   = mul(object_to_proj_matrix, pos);
	output.tbn        = float3x3(t, b, n);
	output.uv         = input.uv;
	output.color      = input.color;
	output.pos_world  = pos_world;
	return output;
}

//
// Helper functions (GGX BRDF)
//
struct BRDF_Surface
{
	float3 normal;
	float  roughness;
	float3 albedo;
	float  metalness;
};

float normal_distribution_ggx(float3 N, float3 H, float roughness)
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

float3 BRDF(BRDF_Surface surface, float3 V, float3 L)
{
	const float3 N         = surface.normal;
	const float  roughness = surface.roughness;
	const float3 albedo    = surface.albedo;
	const float  metallic  = surface.metalness;

	float3 H = normalize(V + L);

	// Fresnel-Schlick approximation for specular reflection.
	float3 F0 = 0.04; 
	F0        = lerp(F0, albedo, metallic);
    float3 F  = F0 + (1.0 - F0) * pow(saturate(1.0 - dot(V, H)), 5.0);

	// Cook-Torrance microfacet BRDF.
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float D     = normal_distribution_ggx(N, H, roughness);
    float G     = geometry_schlick_ggx(NdotV, roughness);
    float denominator = 4.0 * max(NdotL * NdotV, 0.0) + 0.001;
    float3 specular   = (D * F * G) / denominator;

    // Diffuse BRDF.
    float3 kS      = F;
    float3 kD      = (1.0 - kS) * (1.0 - metallic);
    float3 diffuse = (kD * albedo) / PI;

    return diffuse + specular;
}

float4 ps(PS_Input input) : SV_TARGET
{
	// @Note: A good reference for this is "Real Shading in Unreal Engine 4 by Brian Karis, Epic Games".

	float3 albedo_    = base_color.x      < 0.0f? albedo_map   .Sample(sampler0, input.uv).rgb : base_color;
    float3 normal_    = use_normal_map    ==   1? normal_map   .Sample(sampler0, input.uv).rgb : normalize(input.tbn[2]);
	float  metallic_  = metallic          < 0.0f? metallic_map .Sample(sampler0, input.uv).r   : metallic;
	float  roughness_ = roughness         < 0.0f? roughness_map.Sample(sampler0, input.uv).r   : roughness;
	float  ao_        = ambient_occlusion < 0.0f? ao_map       .Sample(sampler0, input.uv).r   : ambient_occlusion;

	//float3 nn = normal_ * 0.5 + 0.5;
	//return float4(nn, 1.0);

	// Convert from sRGB to linear space.
	albedo_ = pow(max(albedo_, 0), 2.2);

	if (use_normal_map == 1) {
		// Convert from tangent space to world space.
		normal_ = normal_ * 2.0 - 1.0;
		normal_ = mul(input.tbn, normal_);
	}

    float3 V = normalize(camera_position - input.pos_world);

    BRDF_Surface surface = (BRDF_Surface)0;
    surface.normal       = normalize(normal_);
    surface.roughness    = roughness_;
	surface.albedo       = albedo_;
	surface.metalness    = metallic_;

	float3 Ia   = 0.3 * surface.albedo * ao_; // Ambient illumination
	float3 IdIs = 0.0;                        // Diffuse and specular illumination

	// Point lights
	// =================================================================================================
    for (int i = 0; i < num_point_lights; i++) {
    	float3 L    = normalize(point_lights[i].position - input.pos_world);
	    float NdotL = saturate(dot(surface.normal, L));
	    
	    // Calculate light radiance/contribution.
	    float dist_to_light = length(point_lights[i].position - input.pos_world);
    	float attenuation   = 1.0 / dist_to_light * dist_to_light;
    	float3 radiance     = attenuation * point_lights[i].color * point_lights[i].intensity;

    	if (dist_to_light < point_lights[i].range)
    		IdIs += BRDF(surface, V, L) * radiance * NdotL;
    }
   	

	// Directional lights
	// =================================================================================================
    for (int j = 0; j < num_dir_lights; j++) {
    	float3 L    = normalize(-dir_lights[j].direction);
    	float NdotL = saturate(dot(surface.normal, L));

	    float3 radiance = dir_lights[j].color * dir_lights[j].intensity;

	    IdIs += BRDF(surface, V, L) * radiance * NdotL;
    }
    
	float3 color = Ia + IdIs;

    // Apply Reinhard tone mapping.
    color = color / (color + 1.0);

    return float4(color, 1.0);
}

)"