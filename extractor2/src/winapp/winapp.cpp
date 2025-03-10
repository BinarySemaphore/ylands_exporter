#include "winapp.hpp"

#include <tchar.h>

HINSTANCE hinst;
int win_width = 500;
int win_height = 500;

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
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
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
		szWindowClass,
		"Ylands Extractor (GUI)",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		win_width, win_height,
		NULL,
		NULL,
		hinst,
		NULL
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

	return (int)msg.wParam;
}

#define ID_BUTTON_ONE 1001
#define ID_LOGBOX_ONE 2001
HWND hlog = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		CreateWindowEx(
			0, "BUTTON", "Run Command",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			5, 5, 150, 30,
			hwnd, (HMENU)ID_BUTTON_ONE, hinst, NULL
		);
		hlog = CreateWindowEx(
			0, "EDIT", NULL,
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL |
			ES_AUTOVSCROLL | ES_READONLY,
			5, 35, win_width - 5, win_height - 35,
			hwnd, (HMENU)ID_LOGBOX_ONE, hinst, NULL
		);
		break;
	case WM_SIZE:
		win_width = LOWORD(lparam);
		win_height = HIWORD(lparam);
		SetWindowPos(hlog, NULL, 0, 0, win_width - 5, win_height - 35, SWP_NOMOVE | SWP_NOZORDER);
		break;
	case WM_COMMAND:
		if (LOWORD(wparam) == ID_BUTTON_ONE) {
			if (!RunCommandAndCaptureOutput("cmd.exe /c ExtractorV2.exe")) {
				int log_length = GetWindowTextLength(hlog);
				SendMessage(hlog, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
				SendMessage(hlog, EM_REPLACESEL, FALSE, (LPARAM)"Failed to execute command.");
			}
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		SetBkMode(hdc, TRANSPARENT);
		EndPaint(hwnd, &ps);
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

    if (!CreateProcess(
        NULL, command, NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL,
        &si, &pi))
    {
        CloseHandle(hWrite);
        CloseHandle(hRead);
		return false;
    }
    CloseHandle(hWrite);

	int log_length;
    TCHAR buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, 1024 - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
		log_length = GetWindowTextLength(hlog);
		SendMessage(hlog, EM_SETSEL, (WPARAM)log_length, (LPARAM)log_length);
		SendMessage(hlog, EM_REPLACESEL, FALSE, (LPARAM)buffer);
    }

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

	return true;
}