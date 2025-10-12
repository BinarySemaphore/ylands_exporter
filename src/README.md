# Ylands Export and Extractor Source

## Requirements
* CMake 3.10.0 or later

### Windows Application (optional - GUI only)
* Win32 + WinRT + UWP dev/lib
  * Easiest method (Windows system or VM):
    1. Install MS Visual Studio 2022 or later (run once - config cmake to use MSVS 2022)
    1. (optional - if having issues) Open Visual Studio Installer after initial install and modify with:
       * Desktop development with C++
       * WinUI application development

## Building
> Note: Recommend using CMake with VSCode
* Use CMake
  * Setup build files (once unless changed `CMakeLists.txt` or switching types `Debug` or `Release`)
    ```
    $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

    ```
  * Build (targets `extractor`, `extractorapp`, or `clean` - no target for build all)
    ```
    $ cmake --build build --target extractor
    ```

## Testing and Debugging
* Example VSCode `launch.json` for debugging
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Run Debug",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}\\build\\Debug\\base\\extractor.exe",
            "args": ["-i", "scene_ref.json", "-t", "OBJ"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}\\build\\Debug\\base",
            "environment": [],
            "console": "integratedTerminal",
        },
        {
            "name": "Run Debug (App)",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}\\build\\Debug\\extractorapp.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}\\build\\Debug\\",
            "environment": []
        }
    ]
}
```

## Documentation
...
