# Ylands Exports

* [Requirements](#requirements)
* [How to Export](#how-to-export)
  * [In Ylands](#in-ylands)
    * [ExportBlockDefs](#exportblockdefs)
    * [ExportScene](#exportscene)
  * [Outside Ylands](#outside-ylands)
    * [Retrieving Exported Data](#retrieving-exported-data)
* [Exported Data Format](#exported-data-format)
  * [Block Defs](#block-defs)
  * [Scenes (flat)](#scenes-flat-)
  * [Scenes (nested)](#scenes-nested-)
* [Reconstruction](#reconstruction)
  * [Ylands Definitions](#ylands-definitions)
    * [Colors](#colors)
    * [Material](#material)
    * [Shape](#shape)
  * [Ylands Coordinate System](#ylands-coordinate-system)
    * [Size](#size)
    * [Order and Compound Transformation](#order-and-compound-transformation)
  * [Reference Scene](#reference-scene)
  * [Example](#example)

## Requirements
* [Python 3.x](#https://www.python.org/downloads/) (*if using extraction script*)

## How to Export
* Add tools to Ylands
  * Use `Ylands` > `Editor` > `Toolbox` > `Import Tool` and import the two files at `./ylands_tools/*.ytool`
    > Alternate: Copy/Move tool files directly into `Steam/userdata/<steam-user-id>/298610/remote/Tools/`<br/>
    > Note: As reference or to recreate tools, the YJS formats of each tool is provided in `./ylands_tools/yjs_format/`

### In Ylands
Always get JSON data before running another Ylands export tool.
See [Retrieving Exported Data](#retrieving-exported-data)

#### ExportBlockDefs
> Note: This only needs to be done once; or never if using the provided `blockdef_2025_02.json`<br/>
> Note: Keep this JSON file as reference data, as it can be used by all exported scenes

1. Open any editor (*preferably an empty/new one*)
1. Run the `ExportBlockDefs` tool
1. Get JSON data before running another export tool (see [Retrieving Exported Data](#retrieving-exported-data))

#### ExportScene
1. Open editor with build you wish to save
1. Run the `ExportScene` tool
1. Get JSON data before running another export tool (see [Retrieving Exported Data](#retrieving-exported-data))

### Outside Ylands
How to get exported JSON data from Ylands after running one of the tools.
Ylands can be left running in the background, it does not need to be exited.
> Note: Do this before running another export tool as it only gets the last logged exported data

#### Retrieving Exported Data
> Note: Script requires Python (see [Requirements](#requirements))

1. Open terminal at location of this `README`
  * For Windows: open the folder, click in the address bar, then type `cmd` and press Enter
1. (*Optional*) Modify `extract_config.json` as needed. `Ylands Install Location` and `Log Location` are required.
1. Run the `export_extractor.py` script to pull, clean, and save the exported JSON data to a file.
```
$ export_extractor.py
```
  * Pass `-h` to print script help and information
   ```
   $ export_extractor.py -h
   ```

## Exported Data Format

### Block Defs
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

### Scenes (flat)
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

### Scenes (nested)
Nested (if `Auto Nest Scenes` is `true` in `extract_config.json`):
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

### Ylands Definitions

#### Colors
Colors are float\[4\] with `Red`, `Green`, `Blue`, and `Alpha`.<br/>
`Alpha` refers to `Emission` luminescence.<br/>
All values range from: `0.0` to `1.0`.

#### Material
String Enum:<br/>
NONE, ALGIMER, BAMBOO, BAROQUE, BRICK, CHEESE, CHOCOLATE, DIAMOND PATTERN, IRON, MARBLE, SANDSTONE, SNOW, STEEL, STONE, STRAW, STRIPED, WOOD

#### Shape
String Enum:<br/>
CORNER, IRREGULAR, LEDGE, ORNAMENT, PILLAR, SLOPE, SPIKE, STAIR, STANDARD, TRIANGULAR, UNDEFINED

### Ylands Coordinate System
Ylands uses Y-vertical with XZ-horizontal.
X+ is to right, Y+ is up, Z+ is forward.

Be sure to transform properly depending on the reconstruction environment.

**Example:**<br/>
Godot uses Y-vertical and XZ-horizontal.
X+ is to the right, Y+ is up, and Z+ is backwards.<br/>
For this simply inverting the Z positions (including for mesh pivot offset) works fine, however rotation is flipped from the Z-axis, so X and Y rotation must be inverted as well.

#### Size
Yland full step/size: `0.375`<br/>
Yland half step/size: `0.1875`

#### Order and Compound Transformation
Ylands positions are all global.  This means even child groups and child objects have global positions.<br/>
If groups are being preserved, then make sure to remove the parent transform from each child group/object:
```
// where rotation is a Quaternion
child.position = parent.rotation.inverse * (child.position - parent.position)
child.rotation = parent.rotation.inverse * child.rotation
```

### Reference Scene
A reference scene in `./ref_scene/` is provided to help ensure your program is reconstructing the exported data correctly.<br/>
`scene_ref_flat.json` is the `flat` reference scene.<br/>
`scene_ref.json` is the `nested` reference scene.

Expected:
* 4x1x1 block rotated up with additional 4x1x1 around the base at various rotations but still inline smooth contact to the bottom of the primary block
* 1x1x1 block rotated over itself, putting pivot above the ground by 1 yland unit
* 2x2x2 block
* Structure Group
  * Copy of structure with group pivot moved to bottom center and group rotated about Y-axis
    * Copy of structure as child group also with pivot moved to its top corner and rotated about XZ-axis
* Musket balls of various colors marking blocks corners (violet glowing ball mark the block's pivot point - at objects position)
  * For groups, the violet musket ball marks a group's pivot/center point

Screenshots are provided as visual example of the reference scene; see `./ref_scene/images/`.<br/>
`00` and `01` show Ylands' camera near starting position.<br/>
All others are close-ups of the objects/groups placed near corners.<br/>
For these, the camera is always facing towards +X and +Z (in Ylands space)<br/>
Musket balls are at object corners, each with different colors for respective block.<br/>
The glowing violet ball is the object's or group's pivot point which should be at the object's listed position.<br/>
> Note: A musket ball's own pivot point is not perfectly centered, it is slightly lower than the sphere's center.  This is not entirely relevant for testing reconstruction.

### Example
Example using Godot (C#/.NET version 4.3)
* Downloads: [Windows](https://godotengine.org/download/windows/) and [Linux](https://godotengine.org/download/linux/)

Direct link to code example [here](https://github.com/BinarySemaphore/ylands_scene_loader/blob/main/scripts/YlandsLoader.cs)<br/>
Project repo [here](https://github.com/BinarySemaphore/ylands_scene_loader)
