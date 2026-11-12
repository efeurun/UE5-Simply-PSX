# Validation

Version 0.1.0, checked on November 12, 2026 with Unreal Engine 5.8.4 on Windows.

The plugin was built and checked in a separate minimal C++ project containing no Noir Night content.

| Check | Result |
| --- | --- |
| Development Editor build, Win64 | Passed |
| Development game build, Win64 | Passed |
| Controller and dependency checks | 27 passed |
| Windows content cook | Passed, zero errors or warnings |
| Screen and example surface materials | Compiled for SM5 and SM6 without fallback materials |
| Asset references | Only SimplyPSX and built-in Unreal resources |

Controller checks cover defaults, independent toggles, all presets, custom resolution, duplicate suppression, ownership transfer, and cleanup. See the [check report](validation.json).

The README images are unedited 1920 × 1080 captures from the Noir Night apartment. The room comparison uses the same camera with PSX disabled and with Subtle 540p. The apartment's custom settings were restored after capture.

This verifies compilation, cooking, asset references, and controller behavior in an editor world. It does not establish packaged-game visual behavior, multiplayer, performance, HDR, VR, split-screen, or compatibility with other engine versions.
