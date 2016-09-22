
float3 GlobalWind(
    float3 vPosition,
    float iTime,
    float2 iPhase,
    float2 iFrequency,
    float2 iAttenuation,
    float2 iBase,
    float2 iMagnitude,
    float3 iWindDirection)
{
    // Preserve shape
    float positionLength = length(vPosition);
                        
    // Compute oscillation
    float4 scaledTime = iTime * iFrequency.xxyy * float4(1.0, 0.8, 1.0, 0.8);
    float4 waves = SmoothTriangleWave(iPhase.xxyy + scaledTime);
    float2 osc = float2(waves.x + waves.y * waves.y, waves.z + waves.w * 0.333);
    float2 value = (iBase + osc * iMagnitude) * iAttenuation;

    // Apply main wind and pulse
    vPosition.xz += iWindDirection.xz * value.x;

    // Restore position
    vPosition = normalize(vPosition) * positionLength;

    // Apply tubulence
    vPosition.y -= value.y;

    return vPosition;
}

float3 FrondEdgeWind(
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
