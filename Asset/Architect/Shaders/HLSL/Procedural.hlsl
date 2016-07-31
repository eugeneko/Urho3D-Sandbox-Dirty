#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "ProceduralImpl.hlsl"

void VS(
    float4 iPos : POSITION,
    float3 iNormal : NORMAL,
    #ifdef NORMALMAP
        float4 iTangent : TANGENT,
    #endif
    #ifndef NOUV
        float4 iTexCoord : TEXCOORD0,
    #endif
    float4 iColor : COLOR0,
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    out float4 oTexCoord : TEXCOORD0,
    #ifdef NORMALMAP
        out float3 oTangent : TEXCOORD3,
        out float3 oBitangent : TEXCOORD4,
    #endif
    out float4 oWorldPos : TEXCOORD2,
    out float3 oNormal : TEXCOORD1,
    out float4 oColor : COLOR0,
    out float4 oPos : OUTPOSITION)
{
    // Define a 0,0 UV coord if not expected from the vertex data
    #ifdef NOUV
        float2 iTexCoord = float2(0.0, 0.0);
    #endif

    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    oWorldPos = float4(worldPos, GetDepth(oPos));
    oNormal = GetWorldNormal(modelMatrix);
    oColor = iColor;

    oTexCoord = iTexCoord;
    #ifdef NORMALMAP
        float3 tangent = GetWorldTangent(modelMatrix);
        float3 bitangent = cross(tangent, oNormal) * iTangent.w;
        oTangent = tangent;
        oBitangent = bitangent;
    #endif
}

void PS(
    float4 iTexCoord : TEXCOORD0,
    #ifdef NORMALMAP
        float3 iTangent : TEXCOORD3,
        float3 iBitangent : TEXCOORD4,
    #endif
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,
    float4 iColor : COLOR0,
    #ifdef DEFERRED
        out float4 oAlbedo : OUTCOLOR1,
        out float4 oNormal : OUTCOLOR2,
        out float4 oDepth : OUTCOLOR3,
    #endif
    out float4 oColor : OUTCOLOR0)
{
    // Get normal
    float3 normal = normalize(iNormal.xyz);
    //float3x3 tbn = float3x3(iTangent.xyz, iBitangent, iNormal);
    //float3 normal = normalize(mul(procNormal.xyz, tbn));

    // Generate procedural textures
    float4 diffColor = 0.0;
    #if defined(MASK)
        float mask = ComputeMask(iTexCoord, iColor);
    #elif defined(SUPERMASK)
        float mask = ComputeSuperMask(iTexCoord);
    #elif defined(MIXCOLOR)
        diffColor = ComputeMixedColor(iTexCoord, iColor);
        //diffColor = 1.0;
    #endif

    #if defined(MASK) || defined(SUPERMASK)
        #ifndef ADDITIVE
            diffColor.rgb = mask;
            diffColor.a = 1.0;
            if (mask == 0.0)
                discard;
        #else
            diffColor.rgb = 1.0;
            diffColor.a = mask;
        #endif
    #endif

    // Fill output
    oColor = diffColor;
    #if defined(DEFERRED)
        oAlbedo = diffColor;
        oNormal = float4(normal * 0.5 + 0.5, 0.0);
        oDepth = iWorldPos.w;
    #endif
}
