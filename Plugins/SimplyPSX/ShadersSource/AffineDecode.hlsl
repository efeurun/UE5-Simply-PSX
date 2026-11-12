// Custom node inputs: UV, Packed (VertexInterpolator output), Enabled, Strength. Float2 output.
// Keep the original coordinate network as UV so a disabled effect restores it exactly.
if (Enabled <= 0.5 || Strength <= 0.0)
    return UV;

float2 affineUV = Packed.xy / max(Packed.z, 0.01);
return lerp(UV, affineUV, saturate(Strength));
