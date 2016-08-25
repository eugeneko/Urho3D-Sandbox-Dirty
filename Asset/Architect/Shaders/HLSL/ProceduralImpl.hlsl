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

/// Compute mask: perlin noise
float ComputeMaskPerlin(float2 iTexCoord, float4 iColor)
{
    float k = iColor.w;
    return PerlinNoise2Dx4(iTexCoord + iColor.z, iColor.xy, float4(1.0, 2.0, 4.0, 8.0), float4(1.0, k, k*k, k*k*k));

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
    else if (type == 1)
    {
        mask = ComputeMaskPerlin(iTexCoord.xy, iColor);
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

/// Compute scaled mask.
/// @param iTexCoord.xy Texture coordinates
/// @param iTexCoord.w  Transparency
/// @param iColor.xy    Mask value is mapped on [begin, end] range
/// @param iColor.zw    Mask value is clamped by [begin, end] range
/// @param TextureUnit0 Mask
float ComputeScaledMask(float4 iTexCoord, float4 iColor)
{
    float4 baseMask = Sample2D(TextureUnit0, iTexCoord.xy);
    return clamp(lerp(iColor.x, iColor.y, baseMask.x), iColor.z, iColor.w) * iTexCoord.w;
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

#endif