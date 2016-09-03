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

/// Compute supermask.
/// @param iTexCoord.xy Texture coordinates
/// @param iTexCoord.w  Transparency
float ComputeSuperMask(float4 iTexCoord)
{
    float4 baseMask = Sample2D(TextureUnit0, iTexCoord.xy);
    return baseMask.x * iTexCoord.w;
}

/// Compute perlin noise.
/// @param iTexCoord.xy Texture coordinates
/// @param iColor.xy    Noise scale
/// @param iColor.zw    Noise offset
float ComputePerlinNoise(float4 iTexCoord, float4 iColor)
{
    return PerlinNoise2D((iTexCoord.xy + iColor.zw) * iColor.xy, iColor.xy);
}

/// Compute color using the following formula:
///   m > 0 ? x*(1 - m) + y*m : z
/// @param iTexCoord.xy Texture coordinates
// /// @param iColor       Color modifier
/// @param TextureUnit0 Mask
/// @param TextureUnit1 X channel
/// @param TextureUnit2 Y channel
/// @param TextureUnit3 Z channel
float4 ComputeMixedColor(float4 iTexCoord, float4 iColor)
{
    float4 mask = Sample2D(TextureUnit0, iTexCoord.xy);
    float4 xtex = Sample2D(TextureUnit1, iTexCoord.xy);
    float4 ytex = Sample2D(TextureUnit2, iTexCoord.xy);
    float4 ztex = Sample2D(TextureUnit3, iTexCoord.xy);
    return mask.x > 0 ? lerp(xtex, ytex, mask.x) : ztex;    
}

/// Noise interpolation
float4 NoiseLerp(float4 source0, float4 source1, float factor, float noise, float smoothing)
{
    float scaledFactor = lerp(-2*smoothing, 1.0 + 2*smoothing, factor);
    float blendFactor = smoothstep(scaledFactor - smoothing, scaledFactor + smoothing, noise);
    return lerp(source1, source0, blendFactor);
}

/// Compute color using the following formula:
///   m > 0 ? x*(1 - m) + y*m : z
/// @param iTexCoord.xy Texture coordinates
// /// @param iColor       Color modifier
/// @param TextureUnit0 Mask
/// @param TextureUnit1 X channel
/// @param TextureUnit2 Y channel
/// @param TextureUnit3 Noise
float4 ComputeSuperMixedColor(float4 iTexCoord, float4 iColor)
{
    float4 mask = Sample2D(TextureUnit0, iTexCoord.xy);
    float4 xtex = Sample2D(TextureUnit1, iTexCoord.xy);
    float4 ytex = Sample2D(TextureUnit2, iTexCoord.xy);
    float4 noise = Sample2D(TextureUnit3, iTexCoord.xy);
    return NoiseLerp(xtex, ytex, mask.x, noise.x, 0.2);    
}

/// Compute texture with filled gaps.
/// @param iTexCoord.xy Texture coordinates
/// @param iColor.xy    Size of texel
float4 ComputeFillGap(float4 iTexCoord, float4 iColor)
{
    float4 baseColor = Sample2D(TextureUnit0, iTexCoord.xy);
    float2 ij;
    for (ij.x = -1; ij.x <= 1; ++ij.x)
        for (ij.y = -1; ij.y <= 1; ++ij.y)
        {
            float4 neighborColor = Sample2D(TextureUnit0, iTexCoord.xy + ij * iColor.xy);
            baseColor = baseColor.a < 0.01 ? neighborColor : baseColor;
        }
    return baseColor;
}

#endif
