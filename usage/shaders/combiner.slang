
static const float2 triangles[3] = {
    float2(-1.0,-1.0),// bottom left
    float2( 3.0,-1.0),// bottom right
    float2(-1.0, 3.0) // upper left
};

// 4.0 is for sciccors to work properly.
static const float2 uvs[3] = {
    float2( 0.0, 0.0),// bottom left
    float2( 2.0, 0.0),// bottom right
    float2( 0.0, 2.0) // upper left
};

struct VsOut{
    float4 pos : SV_Position;
    float2 uv;
};

[[vk::binding(0,0)]] Sampler2D color;
[[vk::binding(1,0)]] Sampler2D<uint> id;

[shader("vertex")]
VsOut main(uint vidx: SV_VulkanVertexID){
    VsOut o;
    float2 triangle = triangles[vidx];

    o.pos = float4(triangle, 0.0, 1.0);
    o.uv = uvs[vidx];

    return o;
}

[shader("fragment")]
float4 main(VsOut input){
    uint objectId = id.Sample(input.uv);

    //if (objectId != 0){
    //    return float4(0.0,1.0,0.0,1.0);
    //}else{
    //    return color.Sample(input.uv);
    //}

    return color.Sample(input.uv);
}
