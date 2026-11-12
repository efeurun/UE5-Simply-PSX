// Custom node inputs: Enabled, Strength, GridHeight, PreviousFrame. Float3 WPO output.
// Feed PreviousFrame with PreviousFrameSwitch(CurrentFrame=0, PreviousFrame=1).
// Camera-view matrices preserve camera alignment when shadow passes override generic View matrices.
if (Enabled <= 0.5 || Strength <= 0.0)
    return float3(0.0, 0.0, 0.0);

float3 translatedPosition = GetTranslatedWorldPosition(Parameters);
float4 viewPosition;
float4 clipPosition;
float2 projectionScale;

if (PreviousFrame > 0.5)
{
    viewPosition = mul(float4(translatedPosition, 1.0), ResolvedView.PrevTranslatedWorldToCameraView);
    clipPosition = mul(viewPosition, ResolvedView.PrevViewToClip);
    // The previous projection contains jitter; remove it before snapping, for stable motion vectors.
    clipPosition.xy -= ResolvedView.TemporalAAJitter.zw * clipPosition.w;
    projectionScale = float2(ResolvedView.PrevViewToClip[0][0], ResolvedView.PrevViewToClip[1][1]);
}
else
{
    viewPosition = mul(float4(translatedPosition, 1.0), ResolvedView.TranslatedWorldToCameraView);
    clipPosition = mul(viewPosition, ResolvedView.ViewToClipNoAA);
    projectionScale = float2(ResolvedView.ViewToClipNoAA[0][0], ResolvedView.ViewToClipNoAA[1][1]);
}

if (clipPosition.w <= 0.01 || min(abs(projectionScale.x), abs(projectionScale.y)) < 0.00001)
    return float3(0.0, 0.0, 0.0);

float height = max(GridHeight, 1.0);
// Derive aspect from the camera projection, since a shadow pass has a different viewport rectangle.
float2 grid = float2(max(round(height * abs(projectionScale.y / projectionScale.x)), 1.0), height);
float2 ndc = clipPosition.xy / clipPosition.w;
float2 uv = ndc * 0.5 + 0.5;
float2 snappedNDC = ((floor(uv * grid) + 0.5) / grid) * 2.0 - 1.0;
float2 viewOffset = (snappedNDC - ndc) * clipPosition.w / projectionScale;

float3 worldOffset;
if (PreviousFrame > 0.5)
    worldOffset = mul(float4(viewOffset, 0.0, 0.0), ResolvedView.PrevCameraViewToTranslatedWorld).xyz;
else
    worldOffset = mul(float4(viewOffset, 0.0, 0.0), ResolvedView.CameraViewToTranslatedWorld).xyz;

worldOffset *= saturate(Strength);
// Unreal units are centimeters. Bound displacement to 5 cm, also set on the master material.
worldOffset *= min(1.0, 5.0 / max(length(worldOffset), 0.00001));
return worldOffset;
