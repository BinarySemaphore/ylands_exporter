# Ylands Export and Extractor

* [Summary](#summary)
* [Requirements](#requirements)
* [Download](#download)
* [How To Extract and Export](#how-to-extract-and-export)
  * [In Ylands](#in-ylands)
  * [In Windows](#in-windows)
  * [Demo Video](#demo-video)
* [Features](#features)
  * [Input](#input)
  * [Export Options](#export-options)
  * [Export](#export)
* [Troubleshooting](#troubleshooting)
* [Mentions and Third-Party Software](#mentions-and-third-party-software)
* [Want to Learn More](#want-to-learn-more)

## Summary
Version: 0.3.0

Ylands Editor Tool and Windows Extractor for exporting *your* builds from Ylands.<br/>
Currently supports saving as `JSON` (raw data) and (3d models `OBJ` or `glTF 2.0`).
> Note:<br/>
> It is recommended to export to `JSON` first and keep these files. `JSON` can be reused, even with future updates.<br/>
> The Ylands Editor Tool will not run on other users blueprints, so you can only export your own editor builds or blueprints.

## Requirements
* Ylands
* Windows 8 or later
  * [MS Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version) (both X86 and X64)
  * If not using Windows (see [Want to Learn More](#want-to-learn-more))

## Download
Latest: [v0.3.0](https://github.com/BinarySemaphore/ylands_exporter/releases/tag/v0.3.0)<br/>
Release comes with the Ylands Editor Tool, Windows Extractor, and CLI base program

## How To Extract and Export
* Add tool to Ylands (once)
  * Use `Ylands` > `Editor` > `Toolbox` > `Import Tool` and import `EXPORTSCENE.ytool`
  * Alternate Import: Copy/Move tool file directly into `Steam/userdata/<steam-user-id>/298610/remote/Tools/`

### In Ylands
1. Open editor with build you wish to save
1. Run the `ExportScene` tool
1. Use `Extractor` before exporting another build
   * `Extractor` only gets the last export data found in Ylands
   * Ylands can remain running in the background while using `Extractor`

### In Windows
1. Run the `extractorapp.exe`
1. Configure `Extract / Input` and `Export Options` as needed
1. Then save using `Convert + Save`
> Note:<br/>
> It is recommended to export to `JSON` first and keep these files. `JSON` can be reused, even with future updates.<br/>
> The Windows Application can be ignored if you're more comfortable with CLI: Run `extractor.exe -h` in `./base/` for details.

### Demo Video
[Youtube: Ylands Export Demo](https://youtu.be/uTrcEmVHT3s)<br/>
[![Demo / Walkthrough Video](https://img.youtube.com/vi/uTrcEmVHT3s/mqdefault.jpg)](https://youtu.be/uTrcEmVHT3s)

## Features
### Input
* Ylands Direct Extraction
* JSON (existing extract)

### Export Options
* Type (see [Export](#export))
* Draw Unsupported Entities
  * Ylands has 5k+ entities and not all geometry is supported by this program, but the bounding boxes are known.
  When enabled, this option will draw transparent bounding boxes for any unsupported entities.
  * Transparency percent can be adjusted.
* Combine Related
  * Combine geometry by shared group and material.
  * Recommended for large builds: reduces export complexity.
  * Unless using \"Join Verticies\", individual entity geometry will still be retained.
  * Note: OBJ exports only supports single objects; a combine is always done for OBJ export (grouping in surfaces).
* Remove Internal Faces
  * Only within same material (unless type `OBJ` or `Apply To All` checked).
  * Disabled if `Combine Related` is not enabled.
  * Any faces adjacent and opposite another face are removed. This includes their opposing neighbor's face.
* Join Vertices
  * Only within same material (unless type `OBJ` or `Apply To All` checked).
  * Disabled if `Combine Related` is not enabled.
  * Any vertices sharing a location with another, or within a very small distance, will be reduced to a single vertex. This efectively *hardens* or *joins* Yland entities into a single geometry.
* Apply To All (Planned)
  * For any `Removal Internal Face` or `Join Verticies`.
  * Disabled if both `Remove Internal Faces` and `Join Vertices` are unchecked.
  * Applies that option to all faces / vertices regardless of material grouping.
* Merge Into Single Geometry (Planned)
  * Same as selecting `Removal Internal Face`, `Join Verticies`, and `Apply To All`.
  * Warning: materials will switch to default.

### Export
* JSON
  * Recomended when using `Ylands Direct Extraction`.
  * This is the raw JSON data which can be kept and used for other conversions at a later time. It is geometry independant data: like a description of a scene / build.
* OBJ
  * Wavefront geometry OBJ and MTL (material) files.
  * A ready-to-render/view conversion of Ylands JSON data.
  * Limited by this program's supported geometry.
  * Forces `Combine Related` as surfaces.
* GLTF / GLB 2.0
  * GLTF 2.0 hierarchical geometry and BIN (binary) files.
  * A ready-to-render conversion of Ylands JSON data.
  * Limited by this program's supported geometry.
  * Recommended if wanting to preserve build groups.
  * GLB is single binary file.

## Troubleshooting
* Status error with `Invalid Config ...`
  * Open `./base/config.json` and ensure the following are correct:
    * `Ylands Install Location`: Should be the full path for Ylands install folder
    * `Log Location`: Should be relative path from the `Ylands Install Location` to `log_userscript_ct.txt`
      * `log_userscript_ct.txt` should be in Ylands folder `Ylands_Data`
      * If `log_userscript_ct.txt` does not exist anywhere in Ylands, try restarting the game and ensure you've run the export tool inside Ylands Editor
      > Note: For Window's paths, make sure to use double backslashes `\\`

## Mentions and Third-Party Software
* [nlohmann/json](https://github.com/nlohmann/json)

## Want to Learn More
The original work and foundation for this project.
* Visit the [core](https://github.com/BinarySemaphore/ylands_exporter/tree/main/core)

Interested in modifying the code or building the extractor on another platform?
* Visit the [source](https://github.com/BinarySemaphore/ylands_exporter/tree/main/src)
