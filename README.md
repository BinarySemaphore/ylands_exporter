# Ylands Exports

* [How to Export (Ylands Scripts)](#how-to-export-ylands-scripts-)
  * [ExportBlockDefs](#exportblockdefs)
  * [ExportScene](#exportscene)
  * [Retrieving Exported Data](#retrieving-exported-data)
* [Ylands and Exported Data](#ylands-and-exported-data)
  * [Definitions](#definitions)
  * [Exported JSON Formats](#exported-json-formats)
* [Reconstruction](#reconstruction)
  * [Reference Scene](#reference-scene)
  * [Ylands Coordinate System](#ylands-coordinate-system)
  * [Example](#example)

## How to Export (Ylands Scripts)
* Add tools to Ylands
  * Use `Ylands` > `Editor` > `Toolbox` > `Import Tool` and import the two files at `./ylands_tools/*.ytool`
    > Alternate: Copy/Move tool files directly into `Steam/userdata/<steam-user-id>/298610/remote/Tools/`<br/>
    > Note: As reference or to recreate tools, the YJS formats of each tool is provided in `./ylands_tools/yjs_format/`

### ExportBlockDefs
> Note: This only needs to be done once; or never if using the provided `blockdef_2025_02.json`

* Open any editor *(preferably an empty/new one)*
* Run the `ExportBlockDefs` tool
* Collect exported data and save to JSON file (keep this as reference data can be used by all exported scenes)
  * See [Retrieving Exported Data](#retrieving-exported-data) for how to collect and save exported data
  * Ref Data is preceded by log line `# Export Block Ref (below):`

### ExportScene
* Open editor with build you wish to save
* Run the `ExportScene` tool
* Collect exported data and save to JSON file
  * See [Retrieving Exported Data](#retrieving-exported-data) for how to collect and save exported data
  * Scene Data is preceded by log lines `# Getting Scene Data...` and `# Export Scene (below):`

### Retrieving Exported Data
All exported data is pushed as JSON string via Yland's script logs (located in `Steam/steamapps/common/Ylands/Ylands_Data/log_userscript_ct.txt`).<br/>
Some UTF-8 characters are liable to be in the data, make sure to pass it through ANSI conversion.<br/>
Scene data is *logged* out in batches: 1) Ylands scripts struggle with large strings and 2) huge single log outputs will crash the game.<br/>
Collect all batches and remove `# ` at the start of each line.<br/>
Recommend using `ylands_nest_scene.py` script on the exported scene JSON file to reformat from flat to nested groups + children.
> Note: On Windows [Notepad++](https://notepad-plus-plus.org/) can help with UTF-8 to ANSI and JSON plugins like `JSTool` for pretty-printing or minifying JSON<br/>
> Note: If `log_userscript_ct.txt` becomes too large, exiting Ylands and restarting will clear the log.<br/>
> TODO: Write script to extract `from log_userscript_ct.txt`, force ANSI, nesting, and making children relative pos/rot

## Ylands and Exported Data

### Definitions

#### Colors
Colors are floats\[4\] with `Red`, `Green`, `Blue`, and `Alpha`.<br/>
`Alpha` refers to `Emission` luminescence.<br/>
Values range from: `0.0` to `1.0`.

#### Material
String Enum:<br/>
NONE, ALGIMER, BAMBOO, BAROQUE, BRICK, CHEESE, CHOCOLATE, DIAMOND PATTERN, IRON, MARBLE, SANDSTONE, SNOW, STEEL, STONE, STRAW, STRIPED, WOOD

#### Shape
String Enum:<br/>
CORNER, IRREGULAR, LEDGE, ORNAMENT, PILLAR, SLOPE, SPIKE, STAIR, STANDARD, TRIANGULAR, UNDEFINED

### Exported JSON Formats

#### Exported Block Defs
```
{
    "<entity-uid>": {
        "type": "<type-name>",
        "size": [x, y, z],
        "shape": "<shape-type>",
        "material": "<mat-type>",
        "colors": [[r, g, b, a], [r, g, b, a], [r, g, b, a]],
        "bb-center-offset": [x, y, z],
        "bb-dimensions": [x, y, z]
    },
    ...
}
```

#### Exported Scenes
Flat / Raw:
```
{
    "<scene-uid>": {
        "type": "entity",
        "name": "<name>",
        "parent": "<scene-uid>",
        "blockdef": "<entity-uid>",
        "position": [x, y, z],
        "rotation": [x, y, z, w],
        "colors": [
            [r, g, b, a],
            [r, g, b, a],
            [r, g, b, a]
        ]
    },
    "<scene-uid>": {
        "type": "group",
        "name": "<name>",
        "parent": "<scene-uid>",
        "position": [x, y, z],
        "rotation": [x, y, z, w],
        "bb-center-offset": [x, y, z],
        "bb-dimensions": [x, y, z]
    },
    ...
}
```
Nested (after using `ylands_nest_scene.py` script):
```
{
    "<scene-uid>": {
        "type": "entity",
        "name": "<name>",
        "blockdef": "<entity-uid>",
        "position": [x, y, z],
        "rotation": [x, y, z, w],
        "colors": [
            [r, g, b, a],
            [r, g, b, a],
            [r, g, b, a]
        ]
    },
    "<scene-uid>": {
        "type": "group",
        "name": "<name>",
        "position": [x, y, z],
        "rotation": [x, y, z, w],
        "bb-center-offset": [x, y, z],
        "bb-dimensions": [x, y, z],
        "children": {
            "<scene-uid>": {...},
            ...
        }
    },
    ...
}
```


## Reconstruction
* Load `Block Defs` JSON data as reference
* Iterate over **keys** in an exported `Scene` JSON data
  * If type is `group`
    * Ignore if not keeping groups 
    * If keeping: create a containing transform
      * If **flat** and object has optional `parent`, use that value as **key** to lookup `<scene-uid>` (wait until end to ensure parent exists)
      * If **nested** create children just-in-time to attach to returning parent object
       > Note: positions are global (see [Order and Compound Transformation](#order-and-compound-transformation) for more info)
  * If type is `entity`
    * Lookup details of `Block Defs` using `<blockdef>` value as **key**
    * Create mesh and material based on `Block Defs` details (type, size, material, shape, etc...)
      * Pivot point can be discovered using (one example method):
        ```
        position mesh in own transform offset by: ref[bb-center-offset] / ref[size]
        ```
* Apply item position, rotation, color as needed
    

### Reference Scene
A reference scene in `./ref_scene/` is provided to help ensure your program is reconstructing the exported data correctly.<br/>
`raw_ref.json` is the ANSI/ASCII converted and pretty-printed JSON from the direct Scene Export.<br/>
`scene_ref.json` is the resulting JSON after running `ylands_nest_scene.py` script on `raw_ref.json` nesting all children.

Expected:
* 4x1x1 block rotated up with additional 4x1x1 around the base at various rotations but still inline smooth contact to the bottom of the primary block
* 1x1x1 block rotated over itself, putting pivot above the ground by 1 yland unit
* 2x2x2 block
* Structure Group
  * Copy of structure with group pivot moved to bottom center and group rotated about Y-axis
    * Copy of structure as child group also with pivot moved to its top corner and rotated about XZ-axis
* Musket balls of various colors marking blocks corners (violet glowing ball for the block's pivot point - at objects position)
  * For groups the violet musket balls mark group pivot/center point

Screenshots are provided as visual example of the reference scene; see `./ref_scene/images/`.<br/>
`00` and `01` show Ylands' camera near starting position.<br/>
All others are close-ups of the objects/groups placed near corners.<br/>
For these, the camera is always facing towards +X and +Z (in Ylands space)<br/>
Musket balls are at object corners, each with different colors.<br/>
The glowing violet ball is the object's or group's pivot point which should be at the object's listed position.<br/>
> Note: A musket balls own pivot point is not perfectly centered, it is slightly lower than the sphere's center.  This is not entirely relevant for testing reconstruction.

### Ylands Coordinate System
Ylands uses Y-vertical with XZ-horizontal.<br/>
X+ is to right, Y+ is up, Z+ is forward.

Be sure to transform properly depending on the reconstruction environment.

**Example:**<br/>
Godot uses Y-vertical and XZ-horizontal.<br/>
X+ is to the right, Y+ is up, and Z- is forward.<br/>
For this simply inverting the Z positions (including for mesh pivot offset) works fine, however rotation is flipped from the Z-axis, so X and Y rotation must be inverted as well.

#### Size
Yland full step/size: `0.375`<br/>
Yland half step/size: `0.1875`

#### Order and Compound Transformation
Ylands positions are all global.  This means even child groups and child objects have global positions.<br/>
If groups are being preserved, then make sure to reverse the parent from each child group/object:
```
// where rotation is a Quaternion
child.position = parent.rotation.inverse * (child.position - parent.position)
child.rotation = parent.rotation.inverse * child.rotation
```

### Example
Example using Godot (C#/.NET version 4.3)
* Downloads: [Windows](https://godotengine.org/download/windows/) and [Linux](https://godotengine.org/download/linux/)

Direct link to code example [here](https://github.com/BinarySemaphore/ylands_scene_loader/blob/main/scripts/YlandsLoader.cs)<br/>
Project repo [here](https://github.com/BinarySemaphore/ylands_scene_loader)
