float4 Mod289(float4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 Permute(float4 x)
{
    return Mod289(((x*34.0)+1.0)*x);
}

float4 TaylorInvSqrt(float4 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

float2 Fade(float2 t) 
{
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}

float PerlinNoise2D(float2 iPixel, float2 iRepeat)
{
    float4 Pi = floor(iPixel.xyxy) + float4(0.0, 0.0, 1.0, 1.0);
    float4 Pf = frac(iPixel.xyxy) - float4(0.0, 0.0, 1.0, 1.0);
    Pi = fmod(Pi, iRepeat.xyxy);
    Pi = Mod289(Pi);
    float4 ix = Pi.xzxz;
    float4 iy = Pi.yyww;
    float4 fx = Pf.xzxz;
    float4 fy = Pf.yyww;

    float4 i = Permute(Permute(ix) + iy);

    float4 gx = frac(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    float4 gy = abs(gx) - 0.5 ;
    float4 tx = floor(gx + 0.5);
    gx = gx - tx;

    float2 g00 = float2(gx.x,gy.x);
    float2 g10 = float2(gx.y,gy.y);
    float2 g01 = float2(gx.z,gy.z);
    float2 g11 = float2(gx.w,gy.w);

    float4 norm = TaylorInvSqrt(float4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  

    float n00 = dot(g00, float2(fx.x, fy.x));
    float n10 = dot(g10, float2(fx.y, fy.y));
    float n01 = dot(g01, float2(fx.z, fy.z));
    float n11 = dot(g11, float2(fx.w, fy.w));

    float2 fade_xy = Fade(Pf.xy);
    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), fade_xy.x);
    float n_xy = lerp(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}

float PerlinNoise2Dx4(float2 iPixel, float2 iRepeat, float4 iPeriods, float4 iMagnitude)
{
    float4 ret;
    ret.x = PerlinNoise2D(iPixel * iRepeat * iPeriods.x, iPeriods.x * iRepeat);
    ret.y = PerlinNoise2D(iPixel * iRepeat * iPeriods.y, iPeriods.y * iRepeat);
    ret.z = PerlinNoise2D(iPixel * iRepeat * iPeriods.z, iPeriods.z * iRepeat);
    ret.w = PerlinNoise2D(iPixel * iRepeat * iPeriods.w, iPeriods.w * iRepeat);
    float totalMagnitude = dot(iMagnitude, 1.0);
    return dot(0.5 + 0.5 * ret, iMagnitude) / totalMagnitude;
}
