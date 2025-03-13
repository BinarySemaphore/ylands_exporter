#include "winapp.hpp"

#include <filesystem>
#include <shobjidl.h>
#include <CommCtrl.h>
#include <dwmapi.h>


HINSTANCE hinst;
COLORREF mid_color;
COLORREF bk_color;
COLORREF bkdk_color;
COLORREF bkbt_color;
COLORREF fg_color;
COLORREF fgdk_color;
COLORREF fgbt_color;
HBRUSH hb_bkg;
int color_shift = 30;
int width = 800;
int height = 500;
int min_width = 570;
int min_height = 500;

const wvm::UISettings ui_settings {};
bool dark_mode = IsColorLight(ui_settings.GetColorValue(wvm::UIColorType::Foreground));

int WINAPI WinMain(
	_In_ HINSTANCE hinstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
) {
	WNDCLASSEX wcex;
	winrt::Windows::UI::Color bk_base;
	winrt::Windows::UI::Color fg_base;
	TCHAR szWindowClass[] = "ExtractorV2App";
	hinst = hinstance;

	// Force lightmode and swap fore/back ground colors
	if (false) {
		dark_mode = false;
		bk_base = ui_settings.GetColorValue(wvm::UIColorType::Foreground);
		fg_base = ui_settings.GetColorValue(wvm::UIColorType::Background);
	} else {
		bk_base = ui_settings.GetColorValue(wvm::UIColorType::Background);
		fg_base = ui_settings.GetColorValue(wvm::UIColorType::Foreground);
	}

	mid_color = RGB(
		(bk_base.R + fg_base.R) / 2,
		(bk_base.G + fg_base.G) / 2,
		(bk_base.B + fg_base.B) / 2
	);
	if (dark_mode) {
		bk_base.R += color_shift;
		bk_base.G += color_shift;
		bk_base.B += color_shift;
		fg_base.R -= color_shift;
		fg_base.G -= color_shift;
		fg_base.B -= color_shift;
	} else {
		bk_base.R -= color_shift;
		bk_base.G -= color_shift;
		bk_base.B -= color_shift;
		fg_base.R += color_shift;
		fg_base.G += color_shift;
		fg_base.B += color_shift;
	}
	bk_color = RGB(bk_base.R, bk_base.G, bk_base.B);
	bkdk_color = RGB(bk_base.R - color_shift, bk_base.G - color_shift, bk_base.B - color_shift);
	bkbt_color = RGB(bk_base.R + color_shift, bk_base.G + color_shift, bk_base.B + color_shift);
	fg_color = RGB(fg_base.R, fg_base.G, fg_base.B);
	fgdk_color = RGB(fg_base.R - color_shift, fg_base.G - color_shift, fg_base.B - color_shift);
	fgbt_color = RGB(fg_base.R + color_shift, fg_base.G + color_shift, fg_base.B + color_shift);
	hb_bkg = CreateSolidBrush(bk_color);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hinst;
	wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)hb_bkg;
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
		WS_EX_OVERLAPPEDWINDOW | WS_EX_ACCEPTFILES,
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

	// Use dark mode title bar
	if (dark_mode) {
		BOOL value = TRUE;
		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
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
#define ID_CB_RIF 2003
#define ID_CB_JV 2004
#define ID_CB_AA 2005
#define ID_CB_MRG 2006
#define ID_LOGBOX 3001
HWND hgrp_input;
HWND hgrp_output;
HWND hgrp_run;
HWND hbtn_select;
HWND hbtn_clear;
HWND hout_input;
HWND hlbl_type;
HWND hop_type;
HWND hop_drwuns;
HWND hlbl_trans;
HWND hop_trans;
HWND hop_rif;
HWND hop_jv;
HWND hop_aa;
HWND hop_mrg;
HWND hbtn_execute;
HWND hout_log;
bool option_drawunsup = false;
float option_transparency = 0.5f;
char output_filename[500] = "";
char output_type[100] = "JSON";
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
			"STATIC", " Extract / Input ",
			WS_CHILD | WS_VISIBLE |
			SS_OWNERDRAW,
			margin, margin,
			rwidth / 3 - half_padding, rheight / 3 - half_padding,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hgrp_output = CreateWindowEx(
			0,
			"STATIC", " Export Options ",
			WS_CHILD | WS_VISIBLE |
			SS_OWNERDRAW,
			margin, margin + rheight / 3 + half_padding,
			rwidth / 3 - half_padding, rheight * 2 / 3 - half_padding,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hgrp_run = CreateWindowEx(
			0,
			"STATIC", " Export ",
			WS_CHILD | WS_VISIBLE |
			SS_OWNERDRAW,
			margin + rwidth / 3 + half_padding, margin,
			rwidth * 2 / 3 - half_padding, rheight,
			hwnd, (HMENU)NULL, hinst, NULL
		);

		// Extract / Input Group
		rx = margin;
		ry = margin + 20;
		hbtn_select = CreateWindowEx(
			0,
			"BUTTON", "Select Existing",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP |
			BS_DEFPUSHBUTTON | BS_OWNERDRAW,
			rx + padding, ry + padding,
			115, 30,
			hwnd, (HMENU)ID_BUTTON_LOAD, hinst, NULL
		);
		hbtn_clear = CreateWindowEx(
			0,
			"BUTTON", "Clear Input",
			WS_CHILD | WS_TABSTOP |
			BS_DEFPUSHBUTTON | BS_OWNERDRAW,
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
		hout_input = CreateWindowEx(
			0,
			"STATIC", input_default,
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			SS_CENTER,
			rx + padding, ry + padding + (2 * padding + 30 + 18),
			rwidth / 3 - half_padding - 2 * padding, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);

		// Export Options Group
		rx = margin;
		ry = margin + rheight / 3 + half_padding + 20;
		hlbl_type = CreateWindowEx(
			0,
			"STATIC", "Type:",
			WS_CHILD | WS_VISIBLE,
			rx + padding, ry + padding,
			40, 20,
			hwnd, (HMENU)500, hinst, NULL
		);
		hop_type = CreateWindowEx(
			0,
			"COMBOBOX", "TYPE",
			WS_CHILD | WS_VISIBLE |
			CBS_DROPDOWNLIST | CBS_HASSTRINGS,
			rx + padding + (padding + 40), ry + padding,
			80, 200,
			hwnd, (HMENU)ID_TYPEBOX, hinst, NULL
		);
		for (const std::string& type : types) {
			SendMessage(hop_type, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(type.c_str()));
		}
		SendMessage(hop_type, CB_SETCURSEL, 0, 0);
		hop_drwuns = CreateWindowEx(
			0,
			"BUTTON", "Draw Unsupported Entities",
			WS_CHILD | BS_CHECKBOX,
			rx + padding, ry + padding + (padding + 20),
			200, 20,
			hwnd, (HMENU)ID_CB_DRWUNS, hinst, NULL
		);
		hlbl_trans = CreateWindowEx(
			0,
			"STATIC", "\\--Transparency (50%):",
			WS_CHILD,
			rx + padding + 40, ry + padding + (padding + 2 * 20),
			160, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		hop_trans = CreateWindowEx(
			0,
			TRACKBAR_CLASS, NULL,
			WS_CHILD | TBS_AUTOTICKS | TBS_ENABLESELRANGE,
			rx + padding + 40, ry + padding + (padding + 3 * 20),
			160, 20,
			hwnd, (HMENU)NULL, hinst, NULL
		);
		SendMessage(hop_trans, TBM_SETRANGE, TRUE, MAKELPARAM(0, 20));
		SendMessage(hop_trans, TBM_SETPOS, TRUE, (int)(option_transparency * 20.0f));
		hop_rif = CreateWindowEx(
			0,
			"BUTTON", "Remove Internal Faces",
			WS_CHILD | BS_CHECKBOX | WS_DISABLED,
			rx + padding, ry + padding + (2 * padding + 4 * 20),
			200, 20,
			hwnd, (HMENU)ID_CB_RIF, hinst, NULL
		);
		hop_jv = CreateWindowEx(
			0,
			"BUTTON", "Join Vertices",
			WS_CHILD | BS_CHECKBOX | WS_DISABLED,
			rx + padding, ry + padding + (3 * padding + 5 * 20),
			200, 20,
			hwnd, (HMENU)ID_CB_JV, hinst, NULL
		);
		hop_aa = CreateWindowEx(
			0,
			"BUTTON", "Apply To All",
			WS_CHILD | BS_CHECKBOX | WS_DISABLED,
			rx + padding, ry + padding + (4 * padding + 6 * 20),
			200, 20,
			hwnd, (HMENU)ID_CB_AA, hinst, NULL
		);
		hop_mrg = CreateWindowEx(
			0,
			"BUTTON", "Merge Into Single Geometry",
			WS_CHILD | BS_CHECKBOX | WS_DISABLED,
			rx + padding, ry + padding + (5 * padding + 7 * 20),
			200, 20,
			hwnd, (HMENU)ID_CB_MRG, hinst, NULL
		);

		// Export Group
		rx = margin + rwidth / 3 + half_padding;
		ry = margin + 20,
		pwidth = rwidth * 2 / 3 - half_padding - (2 * padding);
		pheight = rheight - (2 * padding + 20);
		hbtn_execute = CreateWindowEx(
			0,
			"BUTTON", "Convert + Save As",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP |
			BS_DEFPUSHBUTTON | BS_OWNERDRAW,
			rx + padding, ry + padding,
			135, 30,
			hwnd, (HMENU)ID_BUTTON_EXECUTE, hinst, NULL
		);
		hout_log = CreateWindowEx(
			0,
			"EDIT", "Status:",
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY,
			rx + padding, ry + padding + (padding + 30),
			pwidth, pheight - (padding + 30),
			hwnd, (HMENU)ID_LOGBOX, hinst, NULL
		);

		CreateToolTips(hwnd);
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

		GetWindowRect(hout_log, &r);
		pwidth = r.right - r.left;
		pheight = r.bottom - r.top;
		SetWindowPos(hout_log, NULL, 0, 0, pwidth + rwidth, pheight + rheight, SWP_NOMOVE | SWP_NOZORDER);
		break;
	case WM_COMMAND:
		if (LOWORD(wparam) == ID_BUTTON_LOAD) {
			char fp_name[500];
			if(FileDialogOpenJson(input_filepath, fp_name, 500)) {
				SetWindowText(hout_input, fp_name);
				ShowWindow(hbtn_clear, SW_SHOW);
			}
		} else if (LOWORD(wparam) == ID_BUTTON_EXECUTE) {
			FileDialogSaveAuto(output_filename, 500);

			char c_cmd[1024];
			std::string cmd = "cmd.exe /c ExtractorV2.exe";
			if (output_filename[0] != '\0') {
				std::filesystem::path clean_path = output_filename;
				clean_path.replace_extension("");
				cmd += " -o \"" + clean_path.string() + "\"";
			} else {
				break;
			}
			if (input_filepath[0] != '\0') {
				cmd += " -i \"" + std::string(input_filepath) + "\"";
			}
			if (output_type[0] != '\0') {
				cmd += " -t " + std::string(output_type);
			}
			if (option_drawunsup) {
				cmd += " -u " + std::to_string(option_transparency);
			}
			strcpy_s(c_cmd, cmd.c_str());
			if (!RunCommandAndCaptureOutput(c_cmd)) {
				int log_length = GetWindowTextLength(hout_log);
				SendMessage(hout_log, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
				SendMessage(hout_log, EM_REPLACESEL, FALSE, (LPARAM)"Failed to execute command.");
			}
		} else if (LOWORD(wparam) == ID_BUTTON_CLEAR) {
			ShowWindow(hbtn_clear, SW_HIDE);
			input_filepath[0] = '\0';
			SetWindowText(hout_input, input_default);
		} else if (LOWORD(wparam) == ID_TYPEBOX) {
			int selectedIndex = SendMessage(hop_type, CB_GETCURSEL, 0, 0);
			if (selectedIndex != CB_ERR) {
				SendMessage(hop_type, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(output_type));
				if (strcmp(output_type, "JSON") == 0) {
					ShowWindow(hop_drwuns, SW_HIDE);
					ShowWindow(hlbl_trans, SW_HIDE);
					ShowWindow(hop_trans, SW_HIDE);
					ShowWindow(hop_rif, SW_HIDE);
					ShowWindow(hop_jv, SW_HIDE);
					ShowWindow(hop_aa, SW_HIDE);
					ShowWindow(hop_mrg, SW_HIDE);
				} else {
					ShowWindow(hop_drwuns, SW_SHOW);
					if (SendMessage(hop_drwuns, BM_GETCHECK, 0, 0) == BST_CHECKED) {
						ShowWindow(hlbl_trans, SW_SHOW);
						ShowWindow(hop_trans, SW_SHOW);
					}
					ShowWindow(hop_rif, SW_SHOW);
					ShowWindow(hop_jv, SW_SHOW);
					ShowWindow(hop_aa, SW_SHOW);
					ShowWindow(hop_mrg, SW_SHOW);
				}
			}
		} else if (LOWORD(wparam) == ID_CB_DRWUNS) {
			if (HIWORD(wparam) == BN_CLICKED) {
				if (SendMessage(hop_drwuns, BM_GETCHECK, 0, 0) == BST_CHECKED) {
					SendMessage(hop_drwuns, BM_SETCHECK, BST_UNCHECKED, 0);
					option_drawunsup = false;
					ShowWindow(hlbl_trans, SW_HIDE);
					ShowWindow(hop_trans, SW_HIDE);
				} else {
					 SendMessage(hop_drwuns, BM_SETCHECK, BST_CHECKED, 0);
					 option_drawunsup = true;
					 ShowWindow(hlbl_trans, SW_SHOW);
					 ShowWindow(hop_trans, SW_SHOW);
				}
			}
		}
		break;
	case WM_HSCROLL:
		if ((HWND)lparam == hop_trans) {
			int slider_val = (int)SendMessage(hop_trans, TBM_GETPOS, 0, 0);
			option_transparency = 1.0f - slider_val / 20.0f;
			std::string update_lbl = "\\--Transparency (";
			update_lbl += std::to_string(int(slider_val * 5)) + "%):";
			SetWindowText(hlbl_trans, update_lbl.c_str());
		}
		break;
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC: {
		HDC hdc = (HDC)wparam;
		SetTextColor(hdc, fgdk_color);
		if ((HWND)lparam == hout_log) {
			SetBkColor(hdc, bkdk_color);
		} else {
			SetBkMode(hdc, TRANSPARENT);
			SetBkColor(hdc, bk_color);
		}
		return (LRESULT)hb_bkg;
	}
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLOREDIT: {
		HDC hdc = (HDC)wparam;
		SetTextColor(hdc, fg_color);
		SetBkColor(hdc, bk_color);
		return (LRESULT)hb_bkg;
	}
	case WM_DRAWITEM: {
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		HDC hdc = item->hDC;
		RECT rc = item->rcItem;

		if (item->CtlType == ODT_STATIC) {
			RECT frame = rc;

			HBRUSH b1 = CreateSolidBrush(mid_color);
			frame.top += 8;
			FrameRect(hdc, &frame, b1);
			DeleteObject(b1);

			SetTextColor(hdc, mid_color);
			SetBkMode(hdc, OPAQUE);

			TCHAR text[256];
			GetWindowText(item->hwndItem, text, 255);
			rc.left += 10;
			DrawText(hdc, text, -1, &rc, DT_SINGLELINE);
	
			return TRUE;
		} else if (item->CtlType == ODT_BUTTON) {
			COLORREF ibkg_color = bk_color;
			if (item->itemState & ODS_SELECTED) {
				ibkg_color = bkdk_color;
			}

			HBRUSH b1 = CreateSolidBrush(ibkg_color);
			HBRUSH b2 = CreateSolidBrush(bkbt_color); 
			HBRUSH b3 = CreateSolidBrush(bkdk_color);
			if (item->itemState & ODS_SELECTED) {
				HBRUSH swap = b2;
				b2 = b3;
				b3 = swap;
			}
			// Top Left Bevel
			FillRect(hdc, &rc, b2);
			// Bottom Right Bevel
			OffsetRect(&rc, 2, 2);
			FillRect(hdc, &rc, b3);
			// Center
			OffsetRect(&rc, -2, -2);
			InflateRect(&rc, -2, -2);
			FillRect(hdc, &rc, b1);
			DeleteObject(b1);
			DeleteObject(b2);
			DeleteObject(b3);

			// TODO: Uncomment if tab selection enabled
			// if (item->itemState & ODS_FOCUS) {
			// 	InflateRect(&rc, -4, -4);
			// 	FrameRect(hdc, &rc, (HBRUSH)GetStockObject(mid_color));
			// }

			SetTextColor(hdc, fg_color);
			SetBkMode(hdc, TRANSPARENT);

			TCHAR text[256];
			GetWindowText(item->hwndItem, text, 255);
			DrawText(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	
			return TRUE;
		}
		break;
	}
	case WM_GETMINMAXINFO: {
		MINMAXINFO* minMaxInfo = (MINMAXINFO*)lparam;
		minMaxInfo->ptMinTrackSize.x = min_width;
		minMaxInfo->ptMinTrackSize.y = min_height;
		break;
	}
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
	int log_length = GetWindowTextLength(hout_log);
	SendMessage(hout_log, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
	SendMessage(hout_log, EM_REPLACESEL, FALSE, (LPARAM)text);
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

HWND CreateToolTip(HWND& parent, HWND& item, bool rectAssign, int maxWidth, PTSTR pszText) {
	HWND htip = CreateWindowEx(
		0,
		TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		parent, NULL, hinst, NULL
	);

	if (!htip) return (HWND)NULL;

	TOOLINFO tool_setup = {0};
	tool_setup.cbSize = sizeof(tool_setup);
	tool_setup.uFlags = TTF_SUBCLASS;
	tool_setup.hwnd = parent;
	tool_setup.lpszText = pszText;

	if (!rectAssign) {
		tool_setup.uFlags |= TTF_IDISHWND;
		tool_setup.uId = (UINT_PTR)item;
	} else {
		RECT rect;
		GetWindowRect(item, &rect);
		MapWindowPoints(HWND_DESKTOP, parent, (LPPOINT)&rect, 2);
		tool_setup.rect = rect;
	}

	SendMessage(htip, TTM_ADDTOOL, 0, (LPARAM)&tool_setup);
	SendMessage(htip, TTM_SETMAXTIPWIDTH, 0, maxWidth);
	SendMessage(htip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
	SendMessage(htip, TTM_SETTIPBKCOLOR, bkbt_color, 0);
	SendMessage(htip, TTM_SETTIPTEXTCOLOR, fgbt_color, 0);

	return htip;
}

void CreateToolTips(HWND& parent) {
	int max_width = 400;
	
	// Extract / Input Group
	CreateToolTip(parent, hbtn_select, false, max_width,
"(OPTIONAL) Use data from existing JSON file.\n"
"Existing JSON file is previously extracted JSON data from Ylands."
	);
	CreateToolTip(parent, hbtn_clear, false, max_width,
"Reset input to use\n"
"Ylands Direct Extraction."
	);
	CreateToolTip(parent, hout_input, true, max_width,
"Currently selected JSON reference as input.\n"
"Ylands Direct Extraction will try to find JSON data from Ylands if the"
" \"EXPORTSCENE.ytool\" was used.\n"
"Ylands Direct Extraction is configured in \"extract_config.json\"."
	);

	// Export Options Group
	char data_type_tip[] = ""
"Export data type:\n"
"\nJSON\n"
"| Recomended when using \"Ylands Direct Extraction\".\n"
"| This is the raw JSON data which can be kept and used\n"
"| for other conversions at a later time.\n"
"| It is geometry independant data: like a description of a scene / build.\n"
"\nOBJ\n"
"| Wavefront geometry OBJ and MTL (material) files.\n"
"| A ready-to-render conversion of Ylands JSON data.\n"
"| Limited by this program's supported geometry.";
	CreateToolTip(parent, hlbl_type, true, max_width, data_type_tip);
	CreateToolTip(parent, hop_type, false, max_width, data_type_tip);
	CreateToolTip(parent, hop_drwuns, false, max_width,
"Ylands has 5k+ entities and not all geometry is supported by this program,"
" but the bounding boxes are known.\n"
"When enabled, this option will draw transparent bounding boxes for any"
" unsupported entities."
	);
	CreateToolTip(parent, hop_rif, false, max_width,
"Only within same material (unless \"Apply To All\" checked).\n"
"Any faces adjacent and opposite another face are removed.\n"
"This includes their opposing neighbor's face."
	);
	CreateToolTip(parent, hop_jv, false, max_width,
"Only within same material (unless \"Apply To All\" checked).\n"
"Any vertices sharing a location with another, or within\n"
"a very small distance, will be reduced to a single vertex.\n"
"This efectively \"hardens\" or \"joins\" Yland entities\n"
"into a single geometry."
	);
	CreateToolTip(parent, hop_aa, false, max_width,
"For any \"Removal Internal Face\" or \"Join Verticies\".\n"
"Applies that option to all faces / vertices regardless of material grouping."
	);
	CreateToolTip(parent, hop_mrg, false, max_width,
"Same as selecting \"Removal Internal Face\", \"Join Verticies\", and"
" \"Apply To All\"\n"
"Note: OBJ only supports single objects, so a partial merge is always done for"
" OBJ export."
	);

	// Export Group
	CreateToolTip(parent, hbtn_execute, false, max_width,
"Open save window: to select a location and set file name to save extracted or"
" converted data within.\n"
"Will run a program to make the actual extraction or conversion.\n"
"The program's progress and status will be logged below."
	);
}

bool IsColorLight(wrt::Windows::UI::Color& clr) {
	return (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128));
}