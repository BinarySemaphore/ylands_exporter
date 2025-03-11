#include "winapp.hpp"

#include <filesystem>
#include <shobjidl.h>
#include <CommCtrl.h>

HINSTANCE hinst;
int width = 800;
int height = 500;
int min_width = 570;
int min_height = 500;

int WINAPI WinMain(
	_In_ HINSTANCE hinstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
) {
	WNDCLASSEX wcex;
	TCHAR szWindowClass[] = "ExtractorV2App";
	hinst = hinstance;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hinst;
	wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex)) {
		MessageBox(
			NULL,
			"Call to RegisterClassEx failed!",
			"Ylands Extractor (GUI) Error",
			NULL
		);
		return 1;
	}

	HWND hwnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		szWindowClass, "Ylands Extractor (GUI)",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL, NULL, hinst, NULL
	);

	if (!hwnd) {
		MessageBox(
			NULL,
			"Call to CreateWindow failed!",
			"Ylands Extractor (GUI) Error",
			NULL
		);
		return 1;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Load common controls
	InitCommonControls();

	return (int)msg.wParam;
}

#define ID_BUTTON_LOAD 1001
#define ID_BUTTON_CLEAR 1002
#define ID_BUTTON_EXECUTE 1003
#define ID_TYPEBOX 2001
#define ID_CB_DRWUNS 2002
#define ID_LOGBOX 2003
HWND hgrp_input;
HWND hgrp_output;
HWND hgrp_run;
HWND hclear;
HWND hinput;
HWND htype;
HWND hopdrwuns;
HWND hoplbltrans;
HWND hoptrans;
HWND hlog;
bool option_drawunsup = false;
float option_transparency = 0.5f;
char output_filename[500] = "";
char output_type[100] = "";
char input_default[] = "Ylands Direct Extraction";
char input_filepath[500] = "";
std::vector<std::string> types = {"JSON", "OBJ"};

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	int rwidth, rheight, pwidth, pheight, rx, ry;
	int margin = 10;
	int padding = 6;
	int half_padding = padding / 2;
	RECT r;

	switch (message) {
	case WM_GETMINMAXINFO: {
		MINMAXINFO* minMaxInfo = (MINMAXINFO*)lparam;
		minMaxInfo->ptMinTrackSize.x = min_width;
		minMaxInfo->ptMinTrackSize.y = min_height;
		return 0;
	}
	case WM_CTLCOLORSTATIC: {
		HDC hdc = (HDC)wparam;
		if ((HWND)lparam == hoptrans) {
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW - 1);
		} else if ((HWND)lparam == hlog) {
			SetBkColor(hdc, RGB(200, 200, 200));
		} else {
			SetBkMode(hdc, TRANSPARENT);
		}
		return 0;
	}
	case WM_CREATE:
		// Actual dims
		GetClientRect(hwnd, &r);
		width = r.right - r.left;
		height = r.bottom - r.top;

		// Relative dims
		rwidth = width - (2 * margin);
		rheight = height - (2 * margin);

		// Top level groupings
		hgrp_input = CreateWindowEx(
			0,
			"BUTTON", "Extract / Input",
			WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_EX_ACCEPTFILES,
			margin, margin,
			rwidth / 3 - half_padding, rheight / 3 - half_padding,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hgrp_output = CreateWindowEx(
			0,
			"BUTTON", "Export Options",
			WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			margin, margin + rheight / 3 + half_padding,
			rwidth / 3 - half_padding, rheight * 2 / 3 - half_padding,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hgrp_run = CreateWindowEx(
			0,
			"BUTTON", "Export",
			WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
			margin + rwidth / 3 + half_padding, margin,
			rwidth * 2 / 3 - half_padding, rheight,
			hwnd, (HMENU)NULL, hinst, NULL
		);

		// Extract / Input Group
		rx = margin;
		ry = margin + 20;
		CreateWindowEx(
			0,
			"BUTTON", "Select Existing",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			rx + padding, ry + padding,
			115, 30,
			hwnd, (HMENU)ID_BUTTON_LOAD, hinst, NULL
		);
		hclear = CreateWindowEx(
			0,
			"BUTTON", "Clear Input",
			WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
			rx + padding + (padding + 115), ry + padding,
			90, 30,
			hwnd, (HMENU)ID_BUTTON_CLEAR, hinst, NULL
		);
		CreateWindowEx(
			0,
			"STATIC", "Using:",
			WS_CHILD | WS_VISIBLE,
			rx + padding, ry + padding + (padding + 30),
			40, 18,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hinput = CreateWindowEx(
			0,
			"STATIC", input_default,
			WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON,
			rx + padding, ry + padding + (2 * padding + 30 + 18),
			rwidth / 3 - half_padding - 2 * padding, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);

		// Export Options Group
		rx = margin;
		ry = margin + rheight / 3 + half_padding + 20;
		CreateWindowEx(
			0,
			"STATIC", "Type:",
			WS_CHILD | WS_VISIBLE,
			rx + padding, ry + padding,
			40, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		htype = CreateWindowEx(
			0,
			"COMBOBOX", "TYPE",
			WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS,
			rx + padding + (padding + 40), ry + padding,
			80, 200,
			hwnd, (HMENU)ID_TYPEBOX, hinst, NULL
		);
		for (const std::string& type : types) {
			SendMessage(htype, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(type.c_str()));
		}
		SendMessage(htype, CB_SETCURSEL, 0, 0);
		hopdrwuns = CreateWindowEx(
			0,
			"BUTTON", "Draw Unsupported Entities",
			WS_CHILD | BS_CHECKBOX,
			rx + padding, ry + padding + (padding + 20),
			200, 20,
			hwnd, (HMENU)ID_CB_DRWUNS, hinst, NULL
		);
		hoplbltrans = CreateWindowEx(
			0,
			"STATIC", "\\--Transparency (50%):",
			WS_CHILD,
			rx + padding + 40, ry + padding + (padding + 2 * 20),
			160, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hoptrans = CreateWindowEx(
			0,
			TRACKBAR_CLASS, NULL,
			WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE,
			rx + padding + 40, ry + padding + (padding + 3 * 20),
			160, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		SendMessage(hoptrans, TBM_SETRANGE, TRUE, MAKELPARAM(0, 20));
		SendMessage(hoptrans, TBM_SETPOS, TRUE, (int)(option_transparency * 20.0f));

		// Export Group
		rx = margin + rwidth / 3 + half_padding;
		ry = margin + 20,
		pwidth = rwidth * 2 / 3 - half_padding - (2 * padding);
		pheight = rheight - (2 * padding + 20);
		CreateWindowEx(
			0,
			"BUTTON", "Convert + Save As",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			rx + padding, ry + padding,
			135, 30,
			hwnd, (HMENU)ID_BUTTON_EXECUTE, hinst, NULL
		);
		hlog = CreateWindowEx(
			0,
			"EDIT", "Status:",
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY,
			rx + padding, ry + padding + (padding + 30),
			pwidth, pheight - (padding + 30),
			hwnd, (HMENU)ID_LOGBOX, hinst, NULL
		);
		break;
	case WM_SIZE:
		// Using r* as delta
		rwidth = LOWORD(lparam) - width;
		rheight = HIWORD(lparam) - height;

		// Actual dims
		width = LOWORD(lparam);
		height = HIWORD(lparam);

		GetWindowRect(hgrp_run, &r);
		pwidth = r.right - r.left;
		pheight = r.bottom - r.top;
		SetWindowPos(hgrp_run, NULL, 0, 0, pwidth + rwidth, pheight + rheight, SWP_NOMOVE | SWP_NOZORDER);

		GetWindowRect(hlog, &r);
		pwidth = r.right - r.left;
		pheight = r.bottom - r.top;
		SetWindowPos(hlog, NULL, 0, 0, pwidth + rwidth, pheight + rheight, SWP_NOMOVE | SWP_NOZORDER);
		break;
	case WM_COMMAND:
		if (LOWORD(wparam) == ID_BUTTON_LOAD) {
			char fp_name[500];
			if(FileDialogOpenJson(input_filepath, fp_name, 500)) {
				SetWindowText(hinput, fp_name);
				ShowWindow(hclear, SW_SHOW);

			}
		} else if (LOWORD(wparam) == ID_BUTTON_EXECUTE) {
			FileDialogSaveAuto(output_filename, 500);

			char c_cmd[250];
			std::string cmd = "cmd.exe /c ExtractorV2.exe";
			if (output_filename[0] != '\0') {
				cmd += " -o \"" + std::string(output_filename) + "\"";
			} else {
				break;
			}
			if (input_filepath[0] != '\0') {
				cmd += " -i " + std::string(input_filepath);
			}
			if (output_type[0] != '\0') {
				cmd += " -t " + std::string(output_type);
			}
			if (option_drawunsup) {
				cmd += " -u " + std::to_string(option_transparency);
			}
			strcpy_s(c_cmd, cmd.c_str());
			if (!RunCommandAndCaptureOutput(c_cmd)) {
				int log_length = GetWindowTextLength(hlog);
				SendMessage(hlog, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
				SendMessage(hlog, EM_REPLACESEL, FALSE, (LPARAM)"Failed to execute command.");
			}
		} else if (LOWORD(wparam) == ID_BUTTON_CLEAR) {
			ShowWindow(hclear, SW_HIDE);
			input_filepath[0] = '\0';
			SetWindowText(hinput, input_default);
		} else if (LOWORD(wparam) == ID_TYPEBOX) {
			int selectedIndex = SendMessage(htype, CB_GETCURSEL, 0, 0);
            if (selectedIndex != CB_ERR) {
                SendMessage(htype, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(output_type));
				if (strcmp(output_type, "JSON") == 0) {
					ShowWindow(hopdrwuns, SW_HIDE);
				} else {
					ShowWindow(hopdrwuns, SW_SHOW);
				}
            }
		} else if (LOWORD(wparam) == ID_CB_DRWUNS) {
			if (HIWORD(wparam) == BN_CLICKED) {
                if (SendMessage(hopdrwuns, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    SendMessage(hopdrwuns, BM_SETCHECK, BST_UNCHECKED, 0);
					option_drawunsup = false;
					ShowWindow(hoplbltrans, SW_HIDE);
					ShowWindow(hoptrans, SW_HIDE);
                } else {
                     SendMessage(hopdrwuns, BM_SETCHECK, BST_CHECKED, 0);
					 option_drawunsup = true;
					 ShowWindow(hoplbltrans, SW_SHOW);
					 ShowWindow(hoptrans, SW_SHOW);
                }
            }
		}
		break;
	case WM_HSCROLL:
		if ((HWND)lparam == hoptrans) {
			int slider_val = (int)SendMessage(hoptrans, TBM_GETPOS, 0, 0);
			option_transparency = 1.0f - slider_val / 20.0f;
			std::string update_lbl = "\\--Transparency (";
			update_lbl += std::to_string(int(slider_val * 5)) + "%):";
			SetWindowText(hoplbltrans, update_lbl.c_str());
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	return 0;
}

bool RunCommandAndCaptureOutput(char* command) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		return false;
	}
    // Ensure the read handle to the pipe is not inherited.
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = NULL;
    PROCESS_INFORMATION pi;

	WriteToLogBox("\r\n______________________________\r\n");
	WriteToLogBox(command);
	WriteToLogBox("\r\n==============================\r\n");

	char pwd[250];
	GetCurrentDirectory(250, pwd);
	std::filesystem::path core = "core";
	core = pwd / core;
    if (!CreateProcess(
        NULL, command, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, core.string().c_str(),
        &si, &pi))
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
		return false;
    }
    CloseHandle(hWrite);

    TCHAR buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, 1024 - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
		WriteToLogBox(buffer);
    }

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

	return true;
}

void WriteToLogBox(const TCHAR* text) {
	int log_length = GetWindowTextLength(hlog);
	SendMessage(hlog, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
	SendMessage(hlog, EM_REPLACESEL, FALSE, (LPARAM)text);
}

bool FileDialogOpenJson(char* filepath, char* filename, rsize_t byteSize) {
	bool success = false;
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileOpenDialog *pFileOpen;
		hr = CoCreateInstance(
			CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen)
		);
		if (SUCCEEDED(hr)) {
			// Show open dialog box at PWD (default - will not override last used)
			char c_pwd[250];
			wchar_t pwd[250];
			IShellItem *pCurFolder = NULL; 
			GetCurrentDirectory(250, c_pwd);
			mbstowcs_s(NULL, pwd, c_pwd, std::strlen(c_pwd) + 1);
			hr = SHCreateItemFromParsingName((PCWSTR)pwd, NULL, IID_PPV_ARGS(&pCurFolder));
			if (SUCCEEDED(hr))
			{
				pFileOpen->SetDefaultFolder(pCurFolder);
				pCurFolder->Release();
			}
			COMDLG_FILTERSPEC filter[1] = {{L"JSON Files", L"*.json"}};
			pFileOpen->SetFileTypes(1, filter);
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr)) {
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					if (SUCCEEDED(hr)) {
						success = true;
						wcstombs_s(NULL, filepath, byteSize, pszFilePath, byteSize);
						CoTaskMemFree(pszFilePath);
						std::filesystem::path fp = filepath;
						strcpy_s(filename, byteSize, fp.filename().string().c_str());
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
	return success;
}

bool FileDialogSaveAuto(char* filepath, rsize_t byteSize) {
	bool success = false;
	filepath[0] = '\0';
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileSaveDialog *pFileSave;
		// Create the FileSaveDialog object.
		hr = CoCreateInstance(
			CLSID_FileSaveDialog, NULL, CLSCTX_ALL, 
			IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave)
		);
		if (SUCCEEDED(hr)) {
			// Show save dialog box at PWD (default - will not override last used)
			char c_pwd[250];
			wchar_t pwd[250];
			IShellItem *pCurFolder = NULL; 
			GetCurrentDirectory(250, c_pwd);
			mbstowcs_s(NULL, pwd, c_pwd, std::strlen(c_pwd) + 1);
			hr = SHCreateItemFromParsingName((PCWSTR)pwd, NULL, IID_PPV_ARGS(&pCurFolder));
			if (SUCCEEDED(hr))
			{
				pFileSave->SetDefaultFolder(pCurFolder);
				pCurFolder->Release();
			}

			// Auto set filter
			if (strcmp(output_type, "JSON") == 0) {
				COMDLG_FILTERSPEC filter[1] = {{L"JSON Files", L"*.json"}};
				pFileSave->SetFileTypes(1, filter);
			} else if (strcmp(output_type, "OBJ") == 0) {
				COMDLG_FILTERSPEC filter[1] = {{L"OBJ Wavefront", L"*.obj"}};
				pFileSave->SetFileTypes(1, filter);
			}

			hr = pFileSave->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr)) {
				IShellItem *pItem;
				hr = pFileSave->GetResult(&pItem);
				if (SUCCEEDED(hr)) {
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
					if (SUCCEEDED(hr)) {
						success = true;
						wcstombs_s(NULL, filepath, byteSize, pszFilePath, byteSize);
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileSave->Release();
		}
		CoUninitialize();
	}
	return success;
}