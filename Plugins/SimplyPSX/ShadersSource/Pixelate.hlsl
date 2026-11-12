// Custom node inputs: Scene (PostProcessInput0 resource dependency), PixelHeight,
// PixelationEnabled, ColorReductionEnabled, ColorBits, DitheringEnabled, DitherStrength.
// Float3 output. Use Scene Color After Tonemapping in the desktop SDR pipeline.
#if POST_PROCESS_MATERIAL
float2 viewportUV = GetViewportUV(Parameters);
float2 outputSize = max(PostProcessOutput_ViewportSize, float2(1.0, 1.0));
float pixelHeight = clamp(round(PixelHeight), 1.0, outputSize.y);
float2 grid = float2(max(round(pixelHeight * outputSize.x / outputSize.y), 1.0), pixelHeight);
float2 cell = floor(viewportUV * grid);
float2 sampleViewportUV = PixelationEnabled > 0.5 ? (cell + 0.5) / grid : viewportUV;

// Viewport and scene-texture rectangles differ in resized editor windows and after TSR.
float2 sceneUV = ViewportUVToSceneTextureUV(sampleViewportUV, PPI_PostProcessInput0);
sceneUV = ClampSceneTextureUV(sceneUV, PPI_PostProcessInput0);
float3 color = SceneTextureLookup(sceneUV, PPI_PostProcessInput0, false).rgb;

if (ColorReductionEnabled <= 0.5 && DitheringEnabled <= 0.5)
    return color;

float levels = exp2(clamp(round(ColorBits), 2.0, 8.0)) - 1.0;
float threshold = 0.0;
if (DitheringEnabled > 0.5)
{
    // Screen-anchored Bayer pattern: one threshold per virtual pixel, without time noise.
    const float bayer[16] = {
         0.0,  8.0,  2.0, 10.0,
        12.0,  4.0, 14.0,  6.0,
         3.0, 11.0,  1.0,  9.0,
        15.0,  7.0, 13.0,  5.0
    };
    int2 patternCell = (int2)(PixelationEnabled > 0.5 ? cell : floor(viewportUV * outputSize));
    int index = (patternCell.x & 3) + ((patternCell.y & 3) * 4);
    threshold = ((bayer[index] + 0.5) / 16.0 - 0.5) * saturate(DitherStrength);
}

// UE's SDR tonemapper has already produced display-device code values here.
// Applying another linear-to-sRGB conversion would change the original brightness.
// HDR output requires its own output-device-aware palette treatment.
if (ColorReductionEnabled > 0.5)
    return saturate(floor(saturate(color) * levels + 0.5 + threshold) / levels);

// Dithering is independently usable, with amplitude tied to the selected color bit depth.
return saturate(color + threshold / levels);
#else
return Scene.rgb;
#endif
