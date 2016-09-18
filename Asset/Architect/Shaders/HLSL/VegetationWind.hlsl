float3 GlobalWind(
    float3 vModelPosition, 
    float3 vPosition,
    float3 vNormal,
    float fMainAdherence,
    float fBranchAdherence,
    float fWindMain,
    float fWindPulseMagnitude,
    float fWindPulseFrequency,
    float fBranchPhase,
    float fTime, 
    float3 vWindDirection)
{
    // Preserve shape
    float fPositionLength = length(vPosition);
                        
    // Primary oscillation
    float2 vPhases = float2(fBranchPhase, 0.0);
    float2 fScaledTime = fTime * fWindPulseFrequency * float2(1.0, 0.8);
    float4 vOscillations = SmoothTriangleWave(vModelPosition.xyxy + fScaledTime.xyxy + vPhases.xxyy);
    float2 vOsc = vOscillations.xz + vOscillations.yw * vOscillations.yw;
    float2 vPulse = vOsc * fWindPulseMagnitude * 0.5;
    float fMoveAmount = vPulse.x * fBranchAdherence + (fWindMain + vPulse.y) * fMainAdherence;

    // Move xz component
    vPosition.xz += vWindDirection.xz * fMoveAmount;
        
    // Restore position
    return normalize(vPosition) * fPositionLength;
}

float3 BranchTurbulenceWind(
    float3 vModelPosition, 
    float3 vModelUp,
    float3 vPosition,
    float fTurbulence,
    float fFrequency,
    float fBranchPhase,
    float fTime,
    float3 vWindDirection)
{
    float fObjectPhase = dot(vModelPosition, 1.0);
    float4 vWavesIn = fTime * fFrequency + fObjectPhase + fBranchPhase;
    float4 vWaves = SmoothTriangleWave(vWavesIn) * 0.5 + 0.5;
        
    vPosition += vModelUp * vWaves.x * fTurbulence;
    //vPosition += vWindDirection * vWaves.x * fTurbulence;
    return vPosition;
}

float3 FrondEdgeWind(
    float3 vModelPosition, 
    float3 vPosition,
    float3 vNormal,
    float fEdgeMagnitude,
    float fEdgeFrequency,
    float fTime)
{
    float fObjectPhase = dot(vModelPosition, 1.0);
    float fVertexPhase = dot(vPosition, 1.0);
    const float4 vFreq = float4(1.575, 0.793, 0.375, 0.193);
    float4 vWavesIn = vFreq * fTime * fEdgeFrequency + fObjectPhase + fVertexPhase;
    float4 vWaves = SmoothTriangleWave(vWavesIn);

    vPosition += vNormal * dot(vWaves, float4(1.0, 0.5, 0.25, 0.125)) * fEdgeMagnitude;
    return vPosition;
}
