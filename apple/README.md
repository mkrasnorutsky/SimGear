# SimGear iOS / SPM distribution

SPM product **SimGear** contains full SimGearCore and SimGearScene static libraries plus the bundled Expat parser, built from FlightGear SimGear revision `abe3e7c96e08fe1a28941af923e450f566620ab6` with the OneClickPilot iOS/GLES fixes. Source history and original licenses are retained.

Supports iOS 18+, arm64 devices and arm64/x86_64 Simulator. Rendering uses ANGLE GLES3 through the published FlightGear OpenSceneGraph fork. Sound uses the iOS SDK OpenAL framework. Networking uses the published curl/Secure Transport and c-ares packages; LZMA, Boost and FreeType remain separate dependencies. All build inputs are pinned by release URL and SHA-256 in `apple/dependencies.json`; SPM resolves matching exact versions (ANGLE source package 0.1.1 via OSG).

The package retains normal graphics, Canvas/ShaderVG, model/scenery, XML/property, Nasal and sound support. Standalone tools/tests, video recording, GDAL, AeonWave, RTI and CycloneDDS are disabled. The build does not download aircraft or scenery; those are app resources. Public generated headers are included, with architecture-specific configuration retained when needed.

## Build and validate

On macOS with Xcode iOS SDKs, CMake and Python 3.9+:

```sh
python3 apple/scripts/build_xcframework.py --jobs 6
python3 apple/scripts/validate_xcframework.py
```

Validation links XML/property/elevator input, scenery bucket, scene image utilities, HTTP/DNS clients, sound construction and OSG static plugin/serializer registration against every architecture. To execute the smoke test on an already booted Simulator matching the host architecture, use `--run-simulator`. Default validation is compile/link only and does not claim on-device runtime execution. OSG validation glue is copied from the matching OSG package to keep validation independent of neighboring checkouts.

```sh
python3 apple/scripts/validate_spm.py
```

This resolves the package manifest, builds a downstream C++ consumer for all three architectures and checks its Mach-O platforms. Before release, the root manifest points at the local artifact. After release it downloads and verifies the immutable hosted archive.

## Integration

Add `https://github.com/mkrasnorutsky/SimGear.git`, version `0.1.0`, and select **SimGear**. Include normal `<simgear/...>` headers from C++17 code. Call `SimGearSPMInitialize()` before loading models/fonts to retain and register OSG static plugins and serializers. The app owns UIKit lifecycle, EGL contexts/surfaces, audio-session setup and resources. No window or controls are created by this package.

SPM target compiler definitions apply to the package wrapper. Downstream FlightGear builds must retain their existing Boost compatibility definitions and use the shipped configuration headers. Static OSG headers already define the required static library mode. Linking is covered by SPM's transitive dependencies and SDK framework settings.

The curl dependency uses Secure Transport and supports TLS through 1.2; see that package for its capability limits. This step packages the dependency and does not yet replace the libraries in the OneClickPilot Xcode project.

## Release

```sh
python3 apple/scripts/package_release.py --tag 0.1.0
```

Commit `dist/Package.swift` as the root manifest and upload the ZIP, checksums and build manifest to the matching release. To use locally rebuilt binaries, restore `apple/Package.local.swift` as the root manifest. Published archive bytes/checksums remain immutable.
