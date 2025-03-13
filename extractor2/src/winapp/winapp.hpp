#ifndef WINAPP_H
#define WINAPP_H

#include <windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <string>
#include <tchar.h>

namespace wrt = winrt;
namespace wvm = wrt::Windows::UI::ViewManagement;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
bool RunCommandAndCaptureOutput(char* command);
void WriteToLogBox(const TCHAR* text);
bool FileDialogOpenJson(char* filepath, char* filename, rsize_t byteSize);
bool FileDialogSaveAuto(char* filepath, rsize_t byteSize);
HWND CreateToolTip(HWND& parent, HWND& item, bool rectAssign, int maxWidth, PTSTR pszText);
void CreateToolTips(HWND& parent);
bool IsColorLight(wrt::Windows::UI::Color& clr);

#endif // WINAPP_H