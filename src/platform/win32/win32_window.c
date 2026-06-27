#include "../window.h"
#include "../input.h"
#include "../../core/logger.h"
#include <windows.h>
#include <stdlib.h>

struct GwWindowNative {
    HWND hwnd;
    HDC  hdc;
};

static GwKey s_vk_map[256];

static void build_vk_map(void) {
    for (int i = 'A'; i <= 'Z'; i++) s_vk_map[i] = (GwKey)i;
    for (int i = '0'; i <= '9'; i++) s_vk_map[i] = (GwKey)i;
    s_vk_map[VK_SPACE]    = GW_KEY_SPACE;
    s_vk_map[VK_ESCAPE]   = GW_KEY_ESCAPE;
    s_vk_map[VK_RETURN]   = GW_KEY_ENTER;
    s_vk_map[VK_TAB]      = GW_KEY_TAB;
    s_vk_map[VK_BACK]     = GW_KEY_BACKSPACE;
    s_vk_map[VK_DELETE]   = GW_KEY_DELETE;
    s_vk_map[VK_INSERT]   = GW_KEY_INSERT;
    s_vk_map[VK_HOME]     = GW_KEY_HOME;
    s_vk_map[VK_END]      = GW_KEY_END;
    s_vk_map[VK_PRIOR]    = GW_KEY_PAGE_UP;
    s_vk_map[VK_NEXT]     = GW_KEY_PAGE_DOWN;
    s_vk_map[VK_LEFT]     = GW_KEY_LEFT;
    s_vk_map[VK_RIGHT]    = GW_KEY_RIGHT;
    s_vk_map[VK_UP]       = GW_KEY_UP;
    s_vk_map[VK_DOWN]     = GW_KEY_DOWN;
    for (int i = 0; i < 12; i++) s_vk_map[VK_F1 + i] = (GwKey)(GW_KEY_F1 + i);
    s_vk_map[VK_LSHIFT]   = GW_KEY_LEFT_SHIFT;
    s_vk_map[VK_RSHIFT]   = GW_KEY_RIGHT_SHIFT;
    s_vk_map[VK_LCONTROL] = GW_KEY_LEFT_CTRL;
    s_vk_map[VK_RCONTROL] = GW_KEY_RIGHT_CTRL;
    s_vk_map[VK_LMENU]    = GW_KEY_LEFT_ALT;
    s_vk_map[VK_RMENU]    = GW_KEY_RIGHT_ALT;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    GwWindow *w = (GwWindow *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            if (w) w->running = false;
            return 0;

        case WM_SIZE:
            if (w) { w->width = LOWORD(lp); w->height = HIWORD(lp); }
            return 0;

        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            if (w && w->input) {
                GwKey k = s_vk_map[(u8)wp];
                if (k) input_on_key(w->input, k, true);
            }
            return 0;

        case WM_KEYUP: case WM_SYSKEYUP:
            if (w && w->input) {
                GwKey k = s_vk_map[(u8)wp];
                if (k) input_on_key(w->input, k, false);
            }
            return 0;

        case WM_MOUSEMOVE:
            if (w && w->input) input_on_mouse_move(w->input, LOWORD(lp), HIWORD(lp));
            return 0;

        case WM_LBUTTONDOWN: if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_LEFT,   true);  return 0;
        case WM_LBUTTONUP:   if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_LEFT,   false); return 0;
        case WM_RBUTTONDOWN: if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_RIGHT,  true);  return 0;
        case WM_RBUTTONUP:   if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_RIGHT,  false); return 0;
        case WM_MBUTTONDOWN: if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_MIDDLE, true);  return 0;
        case WM_MBUTTONUP:   if (w && w->input) input_on_mouse_button(w->input, GW_MOUSE_MIDDLE, false); return 0;

        case WM_MOUSEWHEEL:
            if (w && w->input) input_on_scroll(w->input, GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

bool gw_window_create(GwWindow *w, const char *title, i32 width, i32 height) {
    build_vk_map();

    HINSTANCE hi = GetModuleHandleA(NULL);
    WNDCLASSEXA wc = {
        .cbSize        = sizeof(wc),
        .style         = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc   = wnd_proc,
        .hInstance     = hi,
        .hCursor       = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = "GridworksWindow",
    };
    RegisterClassExA(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExA(
        0, "GridworksWindow", title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hi, NULL
    );

    if (!hwnd) { LOG_ERROR("CreateWindowExA failed"); return false; }

    w->native          = malloc(sizeof(GwWindowNative));
    w->native->hwnd    = hwnd;
    w->native->hdc     = GetDC(hwnd);
    w->width           = width;
    w->height          = height;
    w->running         = true;

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)w);
    LOG_INFO("Window created: %s (%dx%d)", title, width, height);
    return true;
}

void gw_window_poll(GwWindow *w) {
    if (w->input) input_update(w->input);
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void gw_window_destroy(GwWindow *w) {
    if (!w->native) return;
    ReleaseDC(w->native->hwnd, w->native->hdc);
    DestroyWindow(w->native->hwnd);
    free(w->native);
    w->native  = NULL;
    w->running = false;
}

void *gw_window_get_hwnd(const GwWindow *w) {
    return w->native ? w->native->hwnd : NULL;
}