# Ylands Export and Extractor Source

## Requirements
* CMake 3.10.0 or later
* Win32 + WinRT dev/libs (if building winapp)

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
