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
        #ifdef INSTANCEDATA
            float4 iInstanceData : TEXCOORD7,
        #endif
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
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        out float4 oFade : COLOR1,
    #endif
    out float4 oPos : OUTPOSITION)
{
    // Define default instance data if not expected from engine
    #ifndef INSTANCED
        #ifdef INSTANCEDATA
            float4 iInstanceData = float4(1.0, 1.0, 0.0, 0.0);
        #endif
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
        float proxyFade = ComputeProxyFade(worldNormal, eye, modelUp, iProxyParam);
        float2 texFadeUv = iSize.zw * iProxyParam.w;
    #else
        float proxyFade = 1.0;
        float2 texFadeUv = 1.0;
    #endif

    #ifdef SCREENFADE
        float screenFade = iInstanceData.x;
    #else
        float screenFade = 1.0;
    #endif

    #ifdef TEXFADE
        float texFade = iInstanceData.y;
    #else
        float texFade = 1.0;
    #endif

    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        oFade = float4(screenFade, texFade * proxyFade, texFadeUv);
    #endif

    // Compute position
    #ifdef OBJECTPROXY
        float3 worldPos = proxyFade > 0 ? GetProxyWorldPosition(iPos, iSize.xy, eye, modelUp, modelPosition, 0.3) : 0.0;
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
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        float4 iFade : COLOR1,
    #endif
    #ifndef D3D11
        float2 iFragPos : VPOS,
    #else
        float4 iFragPos : SV_Position,
    #endif
    out float4 oColor : OUTCOLOR0)
{
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        DiscardByFade(iFade, iFragPos.xy);
    #endif

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
