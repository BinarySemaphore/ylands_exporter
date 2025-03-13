# Ylands Export and Extractor

* [Summary](#summary)
* [Requirements](#requirements)
* [Download](#download)
* [How To Extract and Export](#how-to-extract-and-export)
  * [In Ylands](#in-ylands)
  * [In Windows](#in-windows)
* [Features](#features)
  * [Input](#input)
  * [Export Options](#export-options)
  * [Export](#export)
* [Known Issues](#known-issues)
* [Troubleshooting](#troubleshooting)
* [Mentions and Third-Party Software](#mentions-and-third-party-software)
* [Want to Learn More](#want-to-learn-more)

## Summary
Ylands Editor Tool and Windows Extractor for exporting *your* builds from Ylands.<br/>
Currently supports saving as `JSON` (raw data) and `OBJ Wavefront` (3d model).
> Note:<br/>
> It is recommended to export to `JSON` first and keep these files. `JSON` can be reused, even with future updates.<br/>
> The Ylands Editor Tool will not run on other users blueprints, so you can only export your own editor builds or blueprints.

## Requirements
* Ylands
* Windows 8 or later
  * If not using Windows (see [Want to Learn More](#want-to-learn-more))

## Download
Latest: [v0.1.0](#https://github.com/BinarySemaphore/ylands_exporter/releases/tag/v0.1.0)<br/>
Release ZIP files come with the Ylands Editor Tool and Windows Extractor

## How To Extract and Export
* Add tool to Ylands (once)
  * Use `Ylands` > `Editor` > `Toolbox` > `Import Tool` and import `EXPORTSCENE.ytool`
  * Alternate Import: Copy/Move tool file directly into `Steam/userdata/<steam-user-id>/298610/remote/Tools/`

### In Ylands
1. Open editor with build you wish to save
1. Run the `ExportScene` tool
1. Use `Extractor` before running another export tool (see [Retrieving Exported Data](#retrieving-exported-data))

### In Windows
1. Run the `extractorapp.exe`
1. Configure `Extract / Input` and `Export Options` as needed
1. Then save using `Convert + Save`
> Note:<br/>
> It is recommended to export to `JSON` first and keep these files. `JSON` can be reused, even with future updates.<br/>
> The Windows Application can be ignored if you're more comfortable with command line: Run `./base/extractor.exe -h`. 

## Features
### Input
* Ylands Direct Extraction
* JSON (existing extract)

### Export Options
* Draw Unsupported Entities
  * Ylands has 5k+ entities and not all geometry is supported by this program, but the bounding boxes are known.
  When enabled, this option will draw transparent bounding boxes for any unsupported entities.
* 

### Export
* JSON
* OBJ
* GLTF 2.0 (planned)
* GLB (planned)

## Known Issues
* Reference meshes used to construct 3D models have some wonky normals that need to be cleaned up.
* If the Windows Application encounters an error while running the base program, it has no way of interating or fixing the issue directly.

## Troubleshooting
* Status hangs or crash with output:
  * `Invalid Config ...`
    * Open `./base/config.json` and ensure the following are correct:
      * `Ylands Install Location`: Should be the full path for Ylands install folder
      * `Log Location`: Should be relative path from the install location to `log_userscript_ct.txt`
      * If `log_userscript_ct.txt` does not exist in Ylands, try restarting and ensure you've run the export tool inside Ylands

## Mentions and Third-Party Software
* [nlohmann/json](https://github.com/nlohmann/json)

## Want to Learn More
The original work and foundation for this project.
* Visit the [core](https://github.com/BinarySemaphore/ylands_exporter/tree/main/core)

Interested in modifying the code or building the extractor on another platform?
* Visit the [source](https://github.com/BinarySemaphore/ylands_exporter/tree/main/src)