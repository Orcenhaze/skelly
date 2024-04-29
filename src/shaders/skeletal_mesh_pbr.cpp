/* If you wish to use this shader, include it in the shader factory.

*/

struct Vertex_XTBNUCJW
{
    V3 position;
    V3 tangent;
    V3 bitangent;
    V3 normal;
    V2 uv;
    V4 color;
    
    // V4 because MAX_JOINTS_PER_VERTEX is 4.
    V4s joint_ids;
    V4  weights;
};

#define MAX_JOINTS 65
#define VSConstantsFlags_SHOULD_SKIN 0x1
struct PBR_VS_Constants
{
    M4x4 skinning_matrices[MAX_JOINTS];
    M4x4 object_to_proj_matrix;
    M4x4 object_to_world_matrix;
    u32  flags;
};
#define MAX_POINT_LIGHTS 5
#define MAX_DIR_LIGHTS   2
struct Point_Light
{
    V3  position;  // In world space
    f32 intensity; // Unitless
    V3  color;
    f32 range;	 // In world space units
};
struct Directional_Light
{
    V3  direction; // In world space
    f32 intensity; // Unitless
    V3  color;
    f32 indirect_lighting_intensity; // Unitless @Incomplete: Is this used? 
};
struct PBR_PS_Constants
{
    // @Note: HLSL cbuffers pack everything in 16 bytes (Vector4), so we order stuff this way... yikes.
    Point_Light       point_lights[MAX_POINT_LIGHTS];
	Directional_Light dir_lights[MAX_DIR_LIGHTS];
    
    V3  camera_position; // In world space
    b32 use_normal_map;
    
	V3  base_color;
    f32 metallic;
	f32 roughness;
    f32 ambient_occlusion;
    
    s32 num_point_lights;
    s32 num_dir_lights;
};
GLOBAL ID3D11InputLayout  *pbr_input_layout;
GLOBAL ID3D11Buffer       *pbr_vs_cbuffer;
GLOBAL ID3D11Buffer       *pbr_ps_cbuffer;
GLOBAL ID3D11VertexShader *pbr_vs;
GLOBAL ID3D11PixelShader  *pbr_ps;

FUNCTION void create_shader_skeletal_mesh_pbr()
{
    // Input layout.
    D3D11_INPUT_ELEMENT_DESC layout_desc[] = 
    {
        {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XTBNUCJW, position),  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XTBNUCJW, tangent),   D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XTBNUCJW, bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XTBNUCJW, normal),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(Vertex_XTBNUCJW, uv),        D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex_XTBNUCJW, color),     D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"JOINT_IDS", 0, DXGI_FORMAT_R32G32B32A32_SINT,  0, offsetof(Vertex_XTBNUCJW, joint_ids),     D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"WEIGHTS",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex_XTBNUCJW, weights),     D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    //
    // Constant buffers.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = ALIGN_UP(sizeof(PBR_VS_Constants), 16);
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ASSERT(desc.ByteWidth <= 16*D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT);
        
        device->CreateBuffer(&desc, 0, &pbr_vs_cbuffer);
        
        D3D11_BUFFER_DESC desc2 = desc;
        desc2.ByteWidth = ALIGN_UP(sizeof(PBR_PS_Constants), 16);
        ASSERT(desc2.ByteWidth <= 16*D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT);
        
        device->CreateBuffer(&desc2, 0, &pbr_ps_cbuffer);
    }
    
    char const *hlsl_data = 
#include "shaders/skeletal_mesh_pbr.hlsl"
    ;
    d3d11_compile_shader(hlsl_data, str8_length(hlsl_data), layout_desc, ARRAY_COUNT(layout_desc), &pbr_input_layout, &pbr_vs, &pbr_ps);
}

FUNCTION void bind_shader_skeletal_mesh_pbr()
{
    // @Note: Just assuming this is the stuff we will need for this shader, you can leave certain states
    // for the user to set from outside.
    
    // Vertex Shader.
    device_context->VSSetShader(pbr_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    
    // Pixel Shader.
    device_context->PSSetSamplers(0, 1, &sampler_linear);
    device_context->PSSetShader(pbr_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetBlendState(default_blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetDepthStencilState(default_depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
}

FUNCTION void bind_buffers_skeletal_mesh_pbr(ID3D11Buffer *vbo, ID3D11Buffer *ibo)
{
    // Bind Input Assembler.
    device_context->IASetInputLayout(pbr_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(Vertex_XTBNUCJW);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &vbo, &stride, &offset);
    device_context->IASetIndexBuffer(ibo, DXGI_FORMAT_R32_UINT, 0);
}

FUNCTION void upload_vertex_constants_skeletal_mesh_pbr(PBR_VS_Constants const &vs_constants)
{
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(pbr_vs_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    *(PBR_VS_Constants*)mapped.pData = vs_constants;
    device_context->Unmap(pbr_vs_cbuffer, 0);
    device_context->VSSetConstantBuffers(0, 1, &pbr_vs_cbuffer);
}

FUNCTION void upload_pixel_constants_skeletal_mesh_pbr(PBR_PS_Constants const &ps_constants)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(pbr_ps_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    *(PBR_PS_Constants*)mapped.pData = ps_constants;
    device_context->Unmap(pbr_ps_cbuffer, 0);
    device_context->PSSetConstantBuffers(1, 1, &pbr_ps_cbuffer);
}

FUNCTION void set_texture_skeletal_mesh_pbr(s32 slot, ID3D11ShaderResourceView **texture_view)
{
    device_context->PSSetShaderResources(slot, 1, texture_view);
}