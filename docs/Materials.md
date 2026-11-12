# Material setup

Pixelation, color reduction, and dithering work on the rendered scene. Vertex wobble and texture warping run in surface materials, so each material using those effects needs the functions below.

Enable **Show Plugin Content** in the Content Browser to find **SimplyPSX Content**.

## Starting with the included material

Create a material instance of `/SimplyPSX/Materials/M_PSXSurface` and assign it to a mesh. This is an opaque, lit PBR material with both PSX functions connected.

| Parameter | Use |
| --- | --- |
| BaseColorTexture | Base color texture, imported as sRGB. |
| BaseColorTint | Multiplies the base color. Defaults to white. |
| RoughnessTexture | Linear roughness texture, using its red channel. Use Masks compression with sRGB off. |
| RoughnessMultiplier | Multiplies roughness. Defaults to 0.75. |
| NormalTexture | Tangent-space normal map, with Unreal's normal-map compression. |
| Metallic | Constant metallic value. Defaults to 0. |
| UVTiling | Texture tiling. Defaults to 1. |
| PSX_WobbleMultiplier | 1 follows the controller; 0 disables wobble on this material. |
| PSX_WarpMultiplier | 1 follows the controller; 0 disables texture warping on this material. |

Intermediate PSX multiplier values reduce the effect. These multipliers don't exclude a material from the full-screen effects.

## Adding the effects to an existing material

**Vertex wobble:** add `MF_PSXVertexSnap`. Connect its **Offset** output to **World Position Offset**. Its **Multiplier** input defaults to 1. The function limits displacement to 5 cm; use 5 for the material's **Max World Position Offset Displacement** when this is its only displacement source.

If the material already uses displacement, combine the offsets deliberately and set a bound that includes both. Don't replace existing animation or wind connections accidentally.

**Texture warping:** add `MF_PSXAffineUV`. Feed your texture coordinates into **UV**, then connect its **UV** output to the UV inputs of matching texture samples. Use the same warped coordinates for base color, normals, roughness, and other matching maps.

The function interpolates data from the vertex stage. Its incoming UV calculation must be vertex-compatible: ordinary texture coordinates and tiling work; scene-depth and other pixel-only calculations don't.

Both functions read `/SimplyPSX/Materials/MPC_PSX`, which the controller updates. You don't need to set collection values yourself.

Fine meshes can make affine warping less noticeable. Low-poly surfaces with larger triangles show the effect more clearly. Nanite, translucent materials, complex displacement stacks, and shadow behavior should be checked in the target scene.
