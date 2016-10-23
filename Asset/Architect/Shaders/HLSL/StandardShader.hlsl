#define COMPUTEWORLDNORMAL
#include "StandardCommon.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"
#include "Fog.hlsl"

void VS(float4 iPos : POSITION,
    #if !defined(BILLBOARD) && !defined(TRAILFACECAM)
        float3 iNormal : NORMAL,
    #endif
    #ifndef NOUV
        float2 iTexCoord : TEXCOORD0,
    #endif
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #ifdef WIND
        #ifdef OBJECTPROXY
            float iWindAtten : COLOR1,
        #else
            float4 iWind1 : COLOR1,
            float4 iWind2 : COLOR2,
            float3 iWindNormal : COLOR3,
        #endif
    #endif
    #if defined(LIGHTMAP) || defined(AO)
        float2 iTexCoord2 : TEXCOORD1,
    #endif
    #if defined(NORMALMAP) || defined(TRAILFACECAM) || defined(TRAILBONE)
        float4 iTangent : TANGENT,
    #endif
    #ifdef SKINNED
        float4 iBlendWeights : BLENDWEIGHT,
        int4 iBlendIndices : BLENDINDICES,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
        #ifdef INSTANCEDATA
            float4 iInstanceData : TEXCOORD7,
        #endif
    #endif
    #if defined(BILLBOARD) || defined(DIRBILLBOARD)
        float2 iSize : TEXCOORD1,
    #endif
    #ifdef OBJECTPROXY
        float4 iSize : TEXCOORD1,
        float4 iProxyParam : TEXCOORD2,
    #endif
    #ifndef NORMALMAP
        out float2 oTexCoord : TEXCOORD0,
    #else
        out float4 oTexCoord : TEXCOORD0,
        out float4 oTangent : TEXCOORD3,
    #endif
    out float3 oNormal : TEXCOORD1,
    out float4 oWorldPos : TEXCOORD2,
    #ifdef PERPIXEL
        #ifdef SHADOW
            out float4 oShadowPos[NUMCASCADES] : TEXCOORD4,
        #endif
        #ifdef SPOTLIGHT
            out float4 oSpotPos : TEXCOORD5,
        #endif
        #ifdef POINTLIGHT
            out float3 oCubeMaskVec : TEXCOORD5,
        #endif
    #else
        out float3 oVertexLight : TEXCOORD4,
        out float4 oScreenPos : TEXCOORD5,
        #ifdef ENVCUBEMAP
            out float3 oReflectionVec : TEXCOORD6,
        #endif
        #if defined(LIGHTMAP) || defined(AO)
            out float2 oTexCoord2 : TEXCOORD7,
        #endif
    #endif
    #ifdef VERTEXCOLOR
        out float4 oColor : COLOR0,
    #endif
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        out float4 oFade : COLOR1,
    #endif
    #if defined(D3D11) && defined(CLIPPLANE)
        out float oClip : SV_CLIPDISTANCE0,
    #endif
    out float4 oPos : OUTPOSITION)
{
    // Define default instance data if not expected from engine
    #ifndef INSTANCED
        #ifdef INSTANCEDATA
            float4 iInstanceData = float4(1.0, 1.0, 0.0, 0.0);
        #endif
    #endif

    // Define a 0,0 UV coord if not expected from the vertex data
    #ifdef NOUV
        float2 iTexCoord = float2(0.0, 0.0);
    #endif

    // Get matrix and vectors
    float4x3 modelMatrix = iModelMatrix;
    float3 modelPosition = iModelMatrix._m30_m31_m32;
    #ifdef OBJECTPROXY
        float3 modelUp = normalize(iModelMatrix._m10_m11_m12);
        float3 eye = normalize(cCameraPos - modelPosition);
    #endif

    // Compute normal
    oNormal = GetWorldNormal(modelMatrix);

    // Get fade factor
    #ifdef OBJECTPROXY
        float3 worldNormal = GetWorldNormal(modelMatrix);
        float proxyFade = ComputeProxyFade(worldNormal, eye, modelUp, iProxyParam);
        float2 texFadeUv = iSize.zw * iProxyParam.w;
    #else
        float proxyFade = 1.0;
        float2 texFadeUv = 1.0;
    #endif

    #ifdef SCREENFADE
        float screenFade = iInstanceData.x;
    #else
        float screenFade = 1.0;
    #endif

    #ifdef TEXFADE
        float texFade = iInstanceData.y;
    #else
        float texFade = 1.0;
    #endif

    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        oFade = float4(screenFade, texFade * proxyFade, texFadeUv);
    #endif

    // Compute position
    #ifdef OBJECTPROXY
        float3 worldPos = proxyFade > 0 ? GetProxyWorldPosition(iPos, iSize.xy, eye, modelUp, modelPosition, 0.3) : 0.0;
    #else
        float3 worldPos = GetWorldPos(modelMatrix);
    #endif

    // Apply wind
    #ifdef WIND
        const float objectPhase = dot(modelPosition, 1.0);
        #ifdef OBJECTPROXY
            worldPos = ComputeWind(worldPos, iWindAtten, modelPosition);
        #else
            const float3 worldWindNormal = normalize(mul(iWindNormal, (float3x3)modelMatrix));
            worldPos = ComputeWind(worldPos, iWind1, iWind2, worldWindNormal, modelPosition);
        #endif
    #endif

    oPos = GetClipPos(worldPos);
    oWorldPos = float4(worldPos, GetDepth(oPos));

    #if defined(D3D11) && defined(CLIPPLANE)
        oClip = dot(oPos, cClipPlane);
    #endif

    #ifdef VERTEXCOLOR
        oColor = iColor;
    #endif

    #ifdef NORMALMAP
        // Object proxy always have world-space normals
        #ifdef OBJECTPROXY
            float3 tangent = normalize(iModelMatrix._m00_m01_m02);
            float3 bitangent = modelUp;
            oNormal = normalize(iModelMatrix._m20_m21_m22);
        #else
            float3 tangent = GetWorldTangent(modelMatrix);
            float3 bitangent = cross(tangent, oNormal) * iTangent.w;
        #endif

        oTexCoord = float4(GetTexCoord(iTexCoord), bitangent.xy);
        oTangent = float4(tangent, bitangent.z);
    #else
        oTexCoord = GetTexCoord(iTexCoord);
    #endif

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        float4 projWorldPos = float4(worldPos.xyz, 1.0);

        #ifdef SHADOW
            // Shadow projection: transform from world space to shadow space
            GetShadowPos(projWorldPos, oNormal, oShadowPos);
        #endif

        #ifdef SPOTLIGHT
            // Spotlight projection: transform from world space to projector texture coordinates
            oSpotPos = mul(projWorldPos, cLightMatrices[0]);
        #endif

        #ifdef POINTLIGHT
            oCubeMaskVec = mul(worldPos - cLightPos.xyz, (float3x3)cLightMatrices[0]);
        #endif
    #else
        // Ambient & per-vertex lighting
        #if defined(LIGHTMAP) || defined(AO)
            // If using lightmap, disregard zone ambient light
            // If using AO, calculate ambient in the PS
            oVertexLight = float3(0.0, 0.0, 0.0);
            oTexCoord2 = iTexCoord2;
        #else
            oVertexLight = GetAmbient(GetZonePos(worldPos));
        #endif

        #ifdef NUMVERTEXLIGHTS
            for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
                oVertexLight += GetVertexLight(i, worldPos, oNormal) * cVertexLights[i * 3].rgb;
        #endif

        oScreenPos = GetScreenPos(oPos);

        #ifdef ENVCUBEMAP
            oReflectionVec = worldPos - cCameraPos;
        #endif
    #endif
}

void PS(
    #ifndef NORMALMAP
        float2 iTexCoord : TEXCOORD0,
    #else
        float4 iTexCoord : TEXCOORD0,
        float4 iTangent : TEXCOORD3,
    #endif
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,
    #ifdef PERPIXEL
        #ifdef SHADOW
            float4 iShadowPos[NUMCASCADES] : TEXCOORD4,
        #endif
        #ifdef SPOTLIGHT
            float4 iSpotPos : TEXCOORD5,
        #endif
        #ifdef POINTLIGHT
            float3 iCubeMaskVec : TEXCOORD5,
        #endif
    #else
        float3 iVertexLight : TEXCOORD4,
        float4 iScreenPos : TEXCOORD5,
        #ifdef ENVCUBEMAP
            float3 iReflectionVec : TEXCOORD6,
        #endif
        #if defined(LIGHTMAP) || defined(AO)
            float2 iTexCoord2 : TEXCOORD7,
        #endif
    #endif
    #ifdef VERTEXCOLOR
        float4 iColor : COLOR0,
    #endif
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        float4 iFade : COLOR1,
    #endif
    #if defined(D3D11) && defined(CLIPPLANE)
        float iClip : SV_CLIPDISTANCE0,
    #endif
    #ifndef D3D11
        float2 iFragPos : VPOS,
    #else
        float4 iFragPos : SV_Position,
    #endif
    #ifdef TWOSIDED
        bool iIsFrontFace : SV_IsFrontFace,
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
    #if defined(SCREENFADE) || defined(TEXFADE) || defined(PROXYFADE)
        DiscardByFade(iFade, iFragPos.xy);
    #endif

    // Get material diffuse albedo
    #ifdef DIFFMAP
        float4 diffInput = Sample2D(DiffMap, iTexCoord.xy);
        #ifdef ALPHAMASK
            if (diffInput.a < 0.5)
                discard;
        #endif
        float4 diffColor = cMatDiffColor * diffInput;
    #else
        float4 diffColor = cMatDiffColor;
    #endif

    #ifdef VERTEXCOLOR
        diffColor *= iColor;
    #endif

    #ifdef TERRAINNOISE
        // TODO Don't use emissive map
        float3 terrainNoise;
        terrainNoise.x = Sample2D(EmissiveMap, iTexCoord.xy).r;
        terrainNoise.y = Sample2D(EmissiveMap, iTexCoord.xy * 25).r;
        terrainNoise.z = Sample2D(EmissiveMap, iTexCoord.xy * 125).r;
        diffColor *= terrainNoise.x * terrainNoise.y * terrainNoise.z * 4;
    #endif

    // Get material specular albedo
    #ifdef SPECMAP
        float3 specColor = cMatSpecColor.rgb * Sample2D(SpecMap, iTexCoord.xy).rgb;
    #else
        float3 specColor = cMatSpecColor.rgb;
    #endif

    // Get normal
    #ifdef TWOSIDED
        iNormal *= iIsFrontFace ? 1 : -1;
    #endif
    #ifdef NORMALMAP
        float3x3 tbn = float3x3(iTangent.xyz, float3(iTexCoord.zw, iTangent.w), iNormal);
        float3 normal = normalize(mul(DecodeNormal(Sample2D(NormalMap, iTexCoord.xy)), tbn));
    #else
        float3 normal = normalize(iNormal);
    #endif
    //diffColor.rgb = normal.xyz;

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(iWorldPos.w, iWorldPos.y);
    #else
        float fogFactor = GetFogFactor(iWorldPos.w);
    #endif

    #if defined(PERPIXEL)
        // Per-pixel forward lighting
        float3 lightDir;
        float3 lightColor;
        float3 finalColor;

        float diff = GetDiffuse(normal, iWorldPos.xyz, lightDir);

        #ifdef SHADOW
            diff *= GetShadow(iShadowPos, iWorldPos.w);
        #endif

        #if defined(SPOTLIGHT)
            lightColor = iSpotPos.w > 0.0 ? Sample2DProj(LightSpotMap, iSpotPos).rgb * cLightColor.rgb : 0.0;
        #elif defined(CUBEMASK)
            lightColor = SampleCube(LightCubeMap, iCubeMaskVec).rgb * cLightColor.rgb;
        #else
            lightColor = cLightColor.rgb;
        #endif

        #ifdef SPECULAR
            float spec = GetSpecular(normal, cCameraPosPS - iWorldPos.xyz, lightDir, cMatSpecColor.a);
            finalColor = diff * lightColor * (diffColor.rgb + spec * specColor * cLightColor.a);
        #else
            finalColor = diff * lightColor * diffColor.rgb;
        #endif

        #ifdef AMBIENT
            finalColor += cAmbientColor.rgb * diffColor.rgb;
            finalColor += cMatEmissiveColor;
            oColor = float4(GetFog(finalColor, fogFactor), diffColor.a);
        #else
            oColor = float4(GetLitFog(finalColor, fogFactor), diffColor.a);
        #endif
    #elif defined(PREPASS)
        // Fill light pre-pass G-Buffer
        float specPower = cMatSpecColor.a / 255.0;

        oColor = float4(normal * 0.5 + 0.5, specPower);
        oDepth = iWorldPos.w;
    #elif defined(DEFERRED)
        // Fill deferred G-buffer
        float specIntensity = specColor.g;
        float specPower = cMatSpecColor.a / 255.0;

        float3 finalColor = iVertexLight * diffColor.rgb;
        #ifdef AO
            // If using AO, the vertex light ambient is black, calculate occluded ambient here
            finalColor += Sample2D(EmissiveMap, iTexCoord2).rgb * cAmbientColor.rgb * diffColor.rgb;
        #endif
        #ifdef ENVCUBEMAP
            finalColor += cMatEnvMapColor * SampleCube(EnvCubeMap, reflect(iReflectionVec, normal)).rgb;
        #endif
        #ifdef LIGHTMAP
            finalColor += Sample2D(EmissiveMap, iTexCoord2).rgb * diffColor.rgb;
        #endif
        #ifdef EMISSIVEMAP
            finalColor += cMatEmissiveColor * Sample2D(EmissiveMap, iTexCoord.xy).rgb;
        #else
            finalColor += cMatEmissiveColor;
        #endif

        oColor = float4(GetFog(finalColor, fogFactor), 1.0);
        oAlbedo = fogFactor * float4(diffColor.rgb, specIntensity);
        oNormal = float4(normal * 0.5 + 0.5, specPower);
        oDepth = iWorldPos.w;
    #else
        // Ambient & per-vertex lighting
        float3 finalColor = iVertexLight * diffColor.rgb;
        #ifdef AO
            // If using AO, the vertex light ambient is black, calculate occluded ambient here
            finalColor += Sample2D(EmissiveMap, iTexCoord2).rgb * cAmbientColor.rgb * diffColor.rgb;
        #endif

        #ifdef MATERIAL
            // Add light pre-pass accumulation result
            // Lights are accumulated at half intensity. Bring back to full intensity now
            float4 lightInput = 2.0 * Sample2DProj(LightBuffer, iScreenPos);
            float3 lightSpecColor = lightInput.a * lightInput.rgb / max(GetIntensity(lightInput.rgb), 0.001);

            finalColor += lightInput.rgb * diffColor.rgb + lightSpecColor * specColor;
        #endif

        #ifdef ENVCUBEMAP
            finalColor += cMatEnvMapColor * SampleCube(EnvCubeMap, reflect(iReflectionVec, normal)).rgb;
        #endif
        #ifdef LIGHTMAP
            finalColor += Sample2D(EmissiveMap, iTexCoord2).rgb * diffColor.rgb;
        #endif
        #ifdef EMISSIVEMAP
            finalColor += cMatEmissiveColor * Sample2D(EmissiveMap, iTexCoord.xy).rgb;
        #else
            finalColor += cMatEmissiveColor;
        #endif

        oColor = float4(GetFog(finalColor, fogFactor), diffColor.a);
    #endif
}
