#include <windows.h>
#include <string>
#include <tchar.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RunCommandAndCaptureOutput(char* command);
void WriteToLogBox(const TCHAR* text);
bool FileDialogOpenJson(char* filepath, char* filename, rsize_t byteSize);
bool FileDialogSaveAuto(char* filepath, rsize_t byteSize);