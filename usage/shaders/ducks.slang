
struct UniformData{
    float4x4 proj;
    float4x4 view;
    float4x4 ducks[3];
    float3 lightpos;
    uint32_t hoveridx;
};

struct VsIn{
    float3 pos;
    float3 normal;
    float2 uv;
};

struct VsOut{
    float4 pos: SV_Position;
    float2 uv;
};

struct FsOut{
    float4 col: SV_Target0;
    uint id: SV_Target1;
}

// push constant cmd
[[vk::push_constant]]
uniform UniformData* udata;

// binding - set
[[vk::binding(0,0)]]
Sampler2D albedos[];

[shader("vertex")]
VsOut main(VsIn in, uint idx : SV_VulkanInstanceID){
    VsOut out;

    out.pos =
        mul( udata->proj,
        mul( udata->view,
        mul( udata->ducks[idx], float4(in.pos.xyz, 1.0)
    )));

    out.uv = in.uv;

    return out;
}


[shader("fragment")]
FsOut main(VsOut in){
    FsOut out;

    out.col = float4(albedos[0].Sample(in.uv).rgb, 1.0);
    out.id = 1;

    return out;
}
