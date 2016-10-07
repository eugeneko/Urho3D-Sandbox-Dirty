#include "StandardCommon.hlsl"

void VS(float4 iPos : POSITION,
    #ifdef OBJECTPROXY
        float3 iNormal : NORMAL,
    #endif
    #ifdef SKINNED
        float4 iBlendWeights : BLENDWEIGHT,
        int4 iBlendIndices : BLENDINDICES,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
        float4 iInstanceData : TEXCOORD7,
    #endif
    #ifdef OBJECTPROXY
        float4 iSize : TEXCOORD1,
        float4 iProxyParam : TEXCOORD2,
    #endif
    #ifndef NOUV
        float2 iTexCoord : TEXCOORD0,
    #endif
    #ifdef WIND
        #ifdef OBJECTPROXY
            float iWindAtten : COLOR0,
        #else
            float4 iWind1 : COLOR0,
            float4 iWind2 : COLOR1,
            float3 iWindNormal : COLOR2,
        #endif
    #endif
    #ifdef VSM_SHADOW
        out float3 oTexCoord : TEXCOORD0,
    #else
        out float2 oTexCoord : TEXCOORD0,
    #endif
    out float4 oFade : COLOR1,
    out float4 oPos : OUTPOSITION)
{
    // Define default instance data if not expected from engine
    #ifndef INSTANCED
        float4 iInstanceData = float4(1.0, 0.0, 0.0, 0.0);
    #endif

    // Define a 0,0 UV coord if not expected from the vertex data
    #ifdef NOUV
        float2 iTexCoord = float2(0.0, 0.0);
    #endif

    // Get matrix and vectors
    float4x3 modelMatrix = iModelMatrix;
    float3 modelPosition = iModelMatrix._m30_m31_m32;
    #ifdef OBJECTPROXY
        float3 modelUp = normalize(iModelMatrix._m10_m11_m12);
        float3 eye = normalize(cCameraPos - modelPosition);
    #endif

    // Get fade factor
    #ifdef OBJECTPROXY
        float3 worldNormal = GetWorldNormal(modelMatrix);
        oFade = ComputeFade(iInstanceData.x, 1.0, worldNormal, eye, modelUp, iProxyParam, iSize.zw);
    #else
        oFade = ComputeFade(iInstanceData.x, 1.0, 1.0);
    #endif

    // Compute position
    #ifdef OBJECTPROXY
        float3 worldPos = oFade.y > 0 ? GetProxyWorldPosition(iPos, iSize.xy, eye, modelUp, modelMatrix, 0.3) : 0.0;
    #else
        float3 worldPos = GetWorldPos(modelMatrix);
    #endif

    // Apply wind
    #ifdef WIND
        const float objectPhase = dot(modelPosition, 1.0);
        #ifdef OBJECTPROXY
            worldPos = ComputeWind(worldPos, iWindAtten, modelPosition);
        #else
            const float3 worldWindNormal = normalize(mul(iWindNormal, (float3x3)modelMatrix));
            worldPos = ComputeWind(worldPos, iWind1, iWind2, worldWindNormal, modelPosition);
        #endif
    #endif

    oPos = GetClipPos(worldPos);
    #ifdef VSM_SHADOW
        oTexCoord = float3(GetTexCoord(iTexCoord), oPos.z/oPos.w);
    #else
        oTexCoord = GetTexCoord(iTexCoord);
    #endif
}

void PS(
    #ifdef VSM_SHADOW
        float3 iTexCoord : TEXCOORD0,
    #else
        float2 iTexCoord : TEXCOORD0,
    #endif
    float4 iFade : COLOR1,
    #ifndef D3D11
        float2 iFragPos : VPOS,
    #else
        float4 iFragPos : SV_Position,
    #endif
    out float4 oColor : OUTCOLOR0)
{
    DiscardByFade(iFade, iFragPos.xy);

    #ifdef ALPHAMASK
        float alpha = Sample2D(DiffMap, iTexCoord.xy).a;
        if (alpha < 0.5)
            discard;
    #endif

    #ifdef VSM_SHADOW
        float depth = iTexCoord.z;
        oColor = float4(depth, depth * depth, 1.0, 1.0);
    #else
        oColor = 1.0;
    #endif
}
