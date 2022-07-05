#pragma once

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

typedef std::vector<float_t> vec_t;

#define printf printf2

vector<vector<float>> grid;

#define WINDOW_SIZE_X 640
#define WINDOW_SIZE_Y 480

#define GRID_WINDOW min(WINDOW_SIZE_X, WINDOW_SIZE_Y)

#define GRID_X 28
#define GRID_Y 28

#define GRID_SIZE GRID_WINDOW / GRID_Y

HWND hwnd;
vec_t results;

HBRUSH WHITE = CreateSolidBrush(RGB(255, 255, 255));
HBRUSH GRAY  = CreateSolidBrush(RGB(127, 127, 127));
HBRUSH BLACK = (HBRUSH)GetStockObject(BLACK_BRUSH);

int __cdecl printf2(const char* format, ...) {
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}

void handle_mouse(int x, int y, bool erase = false) {
	int grid_x = ((float)x / GRID_WINDOW) * (GRID_X);
	int grid_y = ((float)y / GRID_WINDOW) * (GRID_Y);

	int left = max(0, grid_x - 1);
	int right = min(27, grid_x + 1);
	int top = max(0, grid_y - 1);
	int bot = min(27, grid_y + 1);

	grid[left][grid_y]  = max(grid[left][grid_y], 0.5f);
	grid[right][grid_y] = max(grid[right][grid_y], 0.5f);
	grid[grid_x][top]   = max(grid[grid_x][top], 0.5f);
	grid[grid_x][bot]   = max(grid[grid_x][bot], 0.5f);
	grid[grid_x][grid_y] = 1.0f;
}

void PaintWindow(HWND hwnd) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	for (int x = 0; x < GRID_X; x++) {
		for (int y = 0; y < GRID_Y; y++) {
			int new_x = GRID_SIZE * x;
			int new_y = GRID_SIZE * y;

			float gray = grid[x][y];
			HBRUSH color = gray == 0.0f ? BLACK : gray == 1.0f ? WHITE : GRAY;

			RECT r{ new_x, new_y, new_x + GRID_SIZE, new_y + GRID_SIZE };
			FillRect(hdc, &r, color);
			
		}
	}

	RECT rect{300, 300, 400, 400};
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkColor(hdc, RGB(0, 0, 0));

	for (int i = 0; i < results.size(); i++) {
		RECT rect{ 500, (WINDOW_SIZE_Y / 2 - 200) + 30 * i, 600, (WINDOW_SIZE_Y / 2 - 100) + 30 * i };

		ostringstream os;
		os << fixed << setprecision(3) << i << " - " << results[i];
		string s = os.str();

		DrawText(hdc, s.c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	}

	EndPaint(hwnd, &ps);
}

void draw_results(vec_t res) {
	results = res;

	RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
}

typedef void (*func)();
func handler;

void bind_handler(func f) {
	handler = f;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static bool down = false;
	static bool erase = false;
	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == 'C') {
			for (int x = 0; x < GRID_X; x++) {
				for (int y = 0; y < GRID_Y; y++) {
					grid[x][y] = 0.0f;
				}
			}
		}

		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
		return 0;
	}
	case WM_CLOSE:   DestroyWindow(hwnd); return 0;
	case WM_DESTROY: PostQuitMessage(0);  return 0;
	case WM_PAINT: {
		PaintWindow(hwnd);
		return 0;
	}
	case WM_LBUTTONDOWN: {
		down = true;
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		handle_mouse(x, y);
		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
		return 0;
	}

	case WM_RBUTTONDOWN: {
		down = true;
		erase = true;
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		handle_mouse(x, y, erase);

		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
		return 0;
	}

	case WM_LBUTTONUP: {
		down = false;
		erase = false;
		handler();
		return 0;
	}
	case WM_RBUTTONUP: {
		down = false;
		erase = false;
		handler();
		return 0;
	}
	case WM_MOUSEMOVE: {
		if (!down) return 0;

		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		handle_mouse(x, y, erase);

		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

const vector<vec_t>& get_grid() {
	return grid;
}

int create_window(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	// Register window class
	WNDCLASSA wc = {
	  0, WndProc, 0, 0, 0,
	  LoadIcon(NULL, IDI_APPLICATION),
	  LoadCursor(NULL, IDC_ARROW),
	  static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)), // background color == black
	  NULL, // no menu
	  "ExampleWindowClass"
	};

	ATOM wClass = RegisterClassA(&wc);
	if (!wClass) {
		fprintf(stderr, "%s\n", "Couldn’t create Window Class");
		return 1;
	}

	// Create the window
	hwnd = CreateWindowA(
		MAKEINTATOM(wClass),
		"Window",     // window title
		WS_OVERLAPPEDWINDOW, // title bar, thick borders, etc.
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_SIZE_X, WINDOW_SIZE_Y,
		NULL, // no parent window
		NULL, // no menu
		GetModuleHandle(NULL),  // EXE's HINSTANCE
		NULL  // no magic user data
	);

	grid.resize(640);
	for (auto& v : grid) v.resize(480);

	if (!hwnd) {
		fprintf(stderr, "%ld\n", GetLastError());
		fprintf(stderr, "%s\n", "Failed to create Window");
		return 1;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);

	return 0;
}