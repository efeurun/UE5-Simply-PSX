// Custom node input: UV. Float3 output, connected to a VertexInterpolator's VS input.
// Perspective interpolation of (UV * clipW, clipW), followed by division in the pixel
// shader, cancels perspective correction and produces the PS1-style affine texture look.
// The snapping effect moves vertices within the camera plane, preserving clip W.
float clipW = max(GetScreenPosition(Parameters).w, 0.01);
return float3(UV * clipW, clipW);
