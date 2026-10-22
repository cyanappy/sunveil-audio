# Sunveil Audio — Originals

Source code for Sunveil Audio's first collection of Windows x64 VST3 guitar effects.

| Pedal | Version | Effect |
| --- | --- | --- |
| [Sundrive](Sundrive/) | 1.3.2 | Transparent and mid-focused overdrive, clipping options, and three-band EQ |
| [Afterglow](Afterglow/) | 1.0.1 | Stereo delay and atmospheric reverb, with Bloom, Shimmer, and Freeze |
| [Glassgarden](Glassgarden/) | 1.0.0 | Granular pitch, stereo clouds, Freeze, and Reverse |

## Source layout

Each pedal is an independent CMake project:

- `Source/`: DSP, VST3 processor/controller, parameters, and native Windows editor.
- `Resources/`: plugin metadata and editable or embedded artwork.
- `Tests/`: host, DSP, and artwork test sources and original validation records.
- `Scripts/`: development or cross-compilation tools.
- `third_party/pluginterfaces/`: vendored VST3 interface sources and their license.
- `ThirdPartyNotices/`: retained dependency and compiler runtime notices.
- `CMakeLists.txt` and `build_windows.bat`: build configuration and Windows build script.

This export includes source and build inputs. Compiled plugins, test executables,
installers, and build outputs are excluded. Original C++ source, artwork, tests,
and build configurations are preserved. Notices added to product documentation
identify instructions that originally accompanied prebuilt release packages.

## Build on Windows x64

The checked-in build scripts target Visual Studio 2022. Install its Desktop
development with C++ workload, Windows SDK, and CMake 3.20 or newer.

Open an x64 developer command prompt in the repository folder. Build a pedal:

```bat
cd Sundrive
build_windows.bat
ctest --test-dir Build -C Release --output-on-failure
```

Repeat from the repository folder with `Afterglow` or `Glassgarden` in place of
`Sundrive`. Afterglow and Glassgarden also run their configured tests within
their build scripts.

| Pedal | Generated VST3 bundle |
| --- | --- |
| Sundrive | `Sundrive/Build/Sundrive.vst3/` |
| Afterglow | `Afterglow/Build/Afterglow.vst3/` |
| Glassgarden | `Glassgarden/Build/Glassgarden.vst3/` |

Each `.vst3` output is a complete bundle directory. Keep its `Contents` directory
and metadata intact when packaging a validated build.

For detailed build notes see [Sundrive/BUILDING.md](Sundrive/BUILDING.md),
[Afterglow/README.md](Afterglow/README.md), and
[Glassgarden/BUILD.md](Glassgarden/BUILD.md). The original `Install.bat` and
`CheckPlugin.bat` files expect a release package layout with compiled binaries;
they are retained as source for those helpers and are not source-build commands.
The projects use C++17 and native Windows editors; JUCE is not required.

## Automated Windows builds

[GitHub Actions](https://github.com/cyanappy/sunveil-audio/actions/workflows/windows-build.yml)
builds each pedal with Visual Studio 2022 on Windows x64 and runs its configured
native tests. Successful runs retain unsigned VST3 validation artifacts with
license notices and source-commit information. These are development artifacts;
installer signing and release publishing are separate steps.

## Code signing policy

The SignPath Foundation application is being prepared. Signing has not been
approved or activated. See the [code signing policy](CODE_SIGNING.md) for build
verification, maintainer roles, manual release approval, and privacy information.

## Validation status

The per-pedal validation documents record checks performed for the original
release packages. Packaging this repository does not rerun those checks. Native
Windows DAW behavior still needs validation. The installer security review is
separate from this source export; no installers are included or cleared here.

## License

Sunveil Audio project code retains its existing MIT license. See [LICENSE.txt](LICENSE.txt)
and each pedal's `LICENSE.txt`. Third-party code and compiler runtime components
retain their own licenses; their notices are preserved in the individual projects.

## Uploading to GitHub

Extract `Sunveil-Audio-Originals-Source.zip`, then add the files and three pedal
folders inside `Sunveil-Audio-Originals-Source` to your repository. Keep the directory
structure intact and commit the extracted files, rather than only the ZIP.
The included `.gitignore` keeps common generated build artifacts out of version control.
