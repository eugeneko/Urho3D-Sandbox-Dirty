/// Get pseudo-random number seeded on given 2D vector.
float Random(float2 iTexCoord)
{
    return frac(sin(dot(iTexCoord.xy, float2(12.9898,78.233))) * 43758.5453);
}

/// Revert linear interpolation between iMin and iMax that resulted iValue.
float UnLerp(float iValue, float iMin, float iMax)
{
    return clamp((iValue - iMin) / (iMax - iMin), 0, 1);
}

/// Project vector on plane with given normal.
float3 ProjectVectorOnPlane(float3 iVec, float3 iNormal)
{
    return normalize(iVec - iNormal * dot(iNormal, iVec));
}

/// Dot product of vectors projected on plane with given normal.
float DotProjected(float3 iFirst, float3 iSecond, float3 iNormal)
{
    return dot(ProjectVectorOnPlane(iFirst, iNormal), ProjectVectorOnPlane(iSecond, iNormal));
}

/// Cubic smooth.
float4 CubicSmooth(float4 iVec)
{
    return iVec * iVec * (3.0 - 2.0 * iVec);
}

/// Triangle wave generator.
float4 TriangleWave(float4 iVec)
{
    return abs((frac(iVec + 0.5) * 2.0) - 1.0);
}

/// Smoothed triangle wave generator.
float4 SmoothTriangleWave(float4 iVec)
{
    return CubicSmooth(TriangleWave(iVec));
}

/// Get world position of proxy vertex.
/// @param iPos Input position, should be the same for all vertices of single billboard.
/// @param iOffset Actual vertex position related to iPos.
/// @param iEye Model to eye direction.
/// @param iModelUp Direction of up direction of the model.
/// @param iModelMatrix Model matrix.
/// @param iMagicFactor Factor that adjusts rotation of degenerate billboards.
float3 GetProxyWorldPosition(float4 iPos, float2 iOffset, float3 iEye, float3 iModelUp, float3 iModelPosition, float iMagicFactor)
{
    float4 right = float4(normalize(cross(iEye, iModelUp)), 0);
    float4 up = float4(normalize(lerp(iModelUp, normalize(cross(right.xyz, iEye)), iMagicFactor)), 0);
    return iPos + iOffset.x * right + iOffset.y * up + iModelPosition;
}

/// Get fade factor of proxy billboard.
/// @param iNormal Normal of billboard
/// @param iEdge Cosine of angle between normal and eye that is semi-faded.
/// @param iThreshold Controls fade region.
/// @param iReverse Whether to reverse fade factor.
/// @param iEye Model to eye direction.
/// @param iModelUp Direction of up direction of the model.
float GetProxyFadeFactor(float3 iNormal, float iEdge, float iThreshold, bool iReverse, float3 iEye, float3 iModelUp)
{
    float normalDot = DotProjected(iEye, iNormal, iModelUp);
    float opacity = UnLerp(normalDot, iEdge - iThreshold, iEdge + iThreshold);
    return iReverse ? 2 - opacity : opacity;
}
