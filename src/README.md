# Ylands Export and Extractor Source

## Requirements
* CMake 3.10.0 or later
* Win32 + WinRT dev/libs (if building winapp)

## Building
* Use CMake to generate build and build files
* Recommend using CMake with VSCode

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
