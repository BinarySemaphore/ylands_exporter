#include <windows.h>
#include <string>
#include <tchar.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
bool RunCommandAndCaptureOutput(char* command);
void WriteToLogBox(const TCHAR* text);
bool FileDialogOpenJson(char* filepath, char* filename, rsize_t byteSize);
bool FileDialogSaveAuto(char* filepath, rsize_t byteSize);
HWND CreateToolTip(HWND& parent, HWND& item, bool rectAssign, int maxWidth, PTSTR pszText);
void CreateToolTips(HWND& parent);