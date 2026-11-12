# Development

The runtime module is `SimplyPSX`. `ASimplyPSXController` controls the post-process effect and the shared material parameter collection.

The plugin includes ready-to-use materials and their HLSL source in `Plugins/SimplyPSX/ShadersSource`. To change a shader, update the corresponding Custom node in the Unreal material or material function, then apply and save. Keep the matching HLSL file in sync; editing that file alone doesn't update the material.

The plugin was checked in a separate minimal project without apartment assets. See [validation results](Validation.md) for the completed checks.
