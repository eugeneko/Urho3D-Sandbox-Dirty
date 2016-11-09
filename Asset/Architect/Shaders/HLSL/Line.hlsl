#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "Fog.hlsl"

#ifdef COMPILEVS
float2 ComputeSide(float2 dir)
{
    float2 rescaled = normalize(dir * cFrustumSize.xy) * cFrustumSize.xy / cFrustumSize.y;
    return float2(rescaled.y, -rescaled.x);
}

float ComputeOrthogonality(float3 start, float3 end)
{
    float3 middlepoint = normalize((start + end)/2.0);
    float3 lineoffset = end - start;
    float3 linedir = normalize(lineoffset);
    return abs(dot(linedir, middlepoint));
}
#endif

void VS(float4 iStartPos : POSITION0,
        float4 iEndPos : POSITION1,
        float2 iTexCoord : TEXCOORD0,
        float2 iOffset : TEXCOORD1,
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    out float2 oTexCoord : TEXCOORD0,
    out float4 oWorldPos : TEXCOORD2,
    #ifdef VERTEXCOLOR
        out float4 oColor : COLOR0,
    #endif
    #if defined(D3D11) && defined(CLIPPLANE)
        out float oClip : SV_CLIPDISTANCE0,
    #endif
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;

    float3 startWorldPos = mul(iStartPos, modelMatrix);
    float3 endWorldPos = mul(iEndPos, modelMatrix);
    float4 startPos = GetClipPos(startWorldPos);
    float4 endPos = GetClipPos(endWorldPos);

    float2 startPos2d = startPos.xy / startPos.w;
    float2 endPos2d = endPos.xy / endPos.w;

    float2 lineDir = normalize(startPos2d - endPos2d);
    float2 lineSide = ComputeSide(lineDir);
    float offsety = max(0, (1 - length(startPos2d - endPos2d)) / 2) * abs(iOffset.y);
    float offsetx = iOffset.x * max(1, 1.5 * startPos.w / iOffset.x / max(cFrustumSize.x, cFrustumSize.y));
    startPos.xy += lineSide * offsetx + lineDir * offsety;
    oPos = startPos;

    oTexCoord = GetTexCoord(iTexCoord);
    oWorldPos = float4(startWorldPos, GetDepth(oPos));

    #if defined(D3D11) && defined(CLIPPLANE)
        oClip = dot(oPos, cClipPlane);
    #endif
    
    #ifdef VERTEXCOLOR
        oColor = iColor;
    #endif
}

void PS(float2 iTexCoord : TEXCOORD0,
    float4 iWorldPos: TEXCOORD2,
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #if defined(D3D11) && defined(CLIPPLANE)
        float iClip : SV_CLIPDISTANCE0,
    #endif
    #ifdef PREPASS
        out float4 oDepth : OUTCOLOR1,
    #endif
    #ifdef DEFERRED
        out float4 oAlbedo : OUTCOLOR1,
        out float4 oNormal : OUTCOLOR2,
        out float4 oDepth : OUTCOLOR3,
    #endif
    out float4 oColor : OUTCOLOR0)
{
    // Get material diffuse albedo
    #ifdef DIFFMAP
        float4 diffColor = cMatDiffColor * Sample2D(DiffMap, iTexCoord);
        #ifdef ALPHAMASK
            if (diffColor.a < 0.5)
                discard;
        #endif
    #else
        float4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= iColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(iWorldPos.w, iWorldPos.y);
    #else
        float fogFactor = GetFogFactor(iWorldPos.w);
    #endif

    #if defined(PREPASS)
        // Fill light pre-pass G-Buffer
        oColor = float4(0.5, 0.5, 0.5, 1.0);
        oDepth = iWorldPos.w;
    #elif defined(DEFERRED)
        // Fill deferred G-buffer
        oColor = float4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
        oAlbedo = float4(0.0, 0.0, 0.0, 0.0);
        oNormal = float4(0.5, 0.5, 0.5, 1.0);
        oDepth = iWorldPos.w;
    #else
        oColor = float4(GetFog(diffColor.rgb, fogFactor), diffColor.a);
    #endif
}
