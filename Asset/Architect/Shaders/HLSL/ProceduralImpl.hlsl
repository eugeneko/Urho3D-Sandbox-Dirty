#ifdef COMPILEPS

#include "Perlin2D.hlsl"

#define TextureUnit0 DiffMap
#define TextureUnit1 NormalMap
#define TextureUnit2 SpecMap
#define TextureUnit3 EmissiveMap

/// Compute mask: segment
// #TODO Write docs
float ComputeMaskSegment(float2 iTexCoord, float4 iColor)
{
    // positivePower < negativePower
    const float positivePower = iColor.x; ///< Positive power of poly
    const float negativePower = iColor.y; ///< Negative power of poly
    const float edgeModifier = iColor.z;  ///< Power to modify intensity at the edge
    const float edgeWidth = iColor.w;     ///< Width of segment at the edge
        
    float xx = abs(iTexCoord.x * 2.0) - edgeWidth * (1.0 - iTexCoord.y);
    float v = pow(iTexCoord.y, positivePower) - pow(iTexCoord.y, negativePower) - xx;
    float ym = pow(positivePower / negativePower, 1 / (negativePower - positivePower));
    float xm = pow(ym, positivePower) - pow(ym, negativePower);
    float p = lerp(edgeModifier, 1.0, iTexCoord.y);
    float value = max(0.0, v) / (xm * p);

    return value;
}

/// Compute mask.
/// @param iTexCoord.xy Texture coordinates
/// @param iTexCoord.z  Mask type
/// @param iTexCoord.w  Transparency
float ComputeMask(float4 iTexCoord, float4 iColor)
{
    float mask = 0.0;
    int type = (int) iTexCoord.z;
 
    // Apply mask
    if (type == 0)
    {
        mask = ComputeMaskSegment(iTexCoord.xy, iColor);
    }

    // Return mask
    return mask * iTexCoord.w;
}

/// Noise interpolation
float4 NoiseLerp(float4 source0, float4 source1, float factor, float noise, float smoothing)
{
    float scaledFactor = lerp(-2*smoothing, 1.0 + 2*smoothing, factor);
    float blendFactor = smoothstep(scaledFactor - smoothing, scaledFactor + smoothing, noise);
    return lerp(source1, source0, blendFactor);
}

#endif
