#include "Constants.hlsl"
#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "Math.hlsl"

#ifdef NOFADE
    #ifdef TEXFADE
        #undef TEXFADE
    #endif
    #ifdef PROXYFADE
        #undef PROXYFADE
    #endif
    #ifdef SCREENFADE
        #undef SCREENFADE
    #endif
#endif

/// Compute fade
#ifdef OBJECTPROXY
    float ComputeProxyFade(float3 iNormal, float3 iEye, float3 iModelUp, float4 iProxyParam)
    {
        return GetProxyFadeFactor(iNormal, iProxyParam.x, iProxyParam.y, iProxyParam.z > 0.0, iEye, iModelUp);
    }
#endif

/// Discard by fade.
#ifdef COMPILEPS
    void DiscardByFade(float4 iFade, float2 iFragPos)
    {
        #if defined(TEXFADE) || defined(PROXYFADE)
            float noiseTexCoord = Random(floor(iFade.zw));
            if (noiseTexCoord > iFade.y || noiseTexCoord + 1.0 < iFade.y)
                discard;
        #endif

        #ifdef SCREENFADE
            float noiseFragPos = Random(floor(iFragPos.xy));
            if (noiseFragPos > iFade.x || noiseFragPos + 1.0 < iFade.x)
                discard;
        #endif
    }
#endif

/// Compute main and turbulence wind.
float3 ComputeGlobalWind(
    float3 iPos,
    float3 iModelPosition,
    float iTime,
    float2 iPhase,
    float2 iFrequency,
    float2 iAttenuation,
    float2 iBase,
    float2 iMagnitude,
    float3 iWindDirection)
{
    // Preserve shape
    iPos -= iModelPosition;
    float positionLength = length(iPos);
                        
    // Compute oscillation
    float4 scaledTime = iTime * iFrequency.xxyy * float4(1.0, 0.8, 1.0, 0.8);
    float4 waves = SmoothTriangleWave(iPhase.xxyy + scaledTime);
    float2 osc = float2(waves.x + waves.y * waves.y, waves.z + waves.w * 0.333);
    float2 value = (iBase + osc * iMagnitude) * iAttenuation;

    // Apply main wind and pulse
    iPos.xz += iWindDirection.xz * value.x;

    // Restore position
    iPos = normalize(iPos) * positionLength + iModelPosition;

    // Apply tubulence
    iPos.y -= value.y;

    return iPos;
}

// Compute foliage wind.
float3 ComputeFoliageWind(
    float3 iPosition,
    float3 iNormal,
    float iTime,
    float iPhase,
    float iMagnitude,
    float iFrequency)
{
    const float4 freq = float4(1.575, 0.793, 0.375, 0.193);
    float4 waves = freq * iTime * iFrequency + iPhase + dot(iPosition, 1.0);
    float4 osc = SmoothTriangleWave(waves) * 2.0 - 1.0;

    return iPosition + iNormal * dot(osc, float4(1.0, 0.5, 0.25, 0.125)) * iMagnitude;
}

#ifdef WIND
    #ifndef D3D11
        // D3D9 uniforms
        uniform float4 cWindDirection;
        uniform float4 cWindParam;
    #else
        // D3D11 constant buffer
        cbuffer WindVS : register(b6)
        {
            float4 cWindDirection;
            float4 cWindParam;
        }
    #endif

    float3 ComputeWind(float3 iPos,
        #ifdef OBJECTPROXY
            float iWindAtten,
        #else
            float4 iWind1,
            float4 iWind2,
            float3 iWindNormal,
        #endif
        float3 iModelPosition)
    {
        const float objectPhase = dot(iModelPosition, 1.0);
        #ifdef OBJECTPROXY
            float3 worldPos = ComputeGlobalWind(
                iPos, // position
                iModelPosition, // model position
                cElapsedTime, // time
                objectPhase, // phase
                float2(cWindParam.w, 0.0), // frequency
                float2(iWindAtten, 0.0), // attenuation
                float2(cWindParam.x, 0.0), // wind base
                cWindParam.zy, // wind magnitude
                cWindDirection);
        #else
            float3 worldPos = ComputeFoliageWind(
                iPos, // position
                iWindNormal, // normal
                cElapsedTime, // time
                objectPhase, // phase
                iWind1.w * max(cWindParam.x, cWindParam.y), // magnitude
                iWind2.y); // frequency
            worldPos = ComputeGlobalWind(
                worldPos, // position
                iModelPosition, // model position
                cElapsedTime, // time
                float2(objectPhase, objectPhase + iWind1.z), // phase
                float2(cWindParam.w, iWind2.x), // frequency
                iWind1.xy, // attenuation
                float2(cWindParam.x, 0.0), // wind base
                cWindParam.zy, // wind magnitude
                cWindDirection);
        #endif
        return worldPos;
    }
#endif
