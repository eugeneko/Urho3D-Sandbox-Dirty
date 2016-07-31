#ifdef COMPILEPS

#define TextureUnit0 DiffMap
#define TextureUnit1 NormalMap
#define TextureUnit2 SpecMap
#define TextureUnit3 EmissiveMap

/// Compute mask (segment)
float ComputeMaskSegment(float4 iTexCoord, float4 iColor)
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
        mask = ComputeMaskSegment(iTexCoord, iColor);
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