#include <windows.h>

namespace
{
    constexpr wchar_t WINDOW_CLASS[] = L"PingoAppWindow";
    constexpr wchar_t WINDOW_TITLE[] = L"Pingo App";

    constexpr COLORREF BACKGROUND = RGB(13, 13, 13);
    constexpr COLORREF SURFACE = RGB(22, 22, 22);
    constexpr COLORREF BORDER = RGB(48, 48, 48);
    constexpr COLORREF TEXT_PRIMARY = RGB(255, 255, 255);
    constexpr COLORREF TEXT_SECONDARY = RGB(165, 165, 165);

    HFONT CreatePingoFont(int size, int weight)
    {
        return CreateFontW(
            size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI Variable"
        );
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT client{};
            GetClientRect(hwnd, &client);

            HBRUSH background = CreateSolidBrush(BACKGROUND);
            FillRect(hdc, &client, background);
            DeleteObject(background);

            SetBkMode(hdc, TRANSPARENT);

            HFONT titleFont = CreatePingoFont(30, FW_SEMIBOLD);
            HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, titleFont));
            SetTextColor(hdc, TEXT_PRIMARY);

            RECT titleRect{32, 28, client.right - 32, 72};
            DrawTextW(hdc, L"Pingo App", -1, &titleRect,
                      DT_LEFT | DT_SINGLELINE | DT_VCENTER);

            SelectObject(hdc, oldFont);
            DeleteObject(titleFont);

            HFONT subtitleFont = CreatePingoFont(16, FW_NORMAL);
            oldFont = static_cast<HFONT>(SelectObject(hdc, subtitleFont));
            SetTextColor(hdc, TEXT_SECONDARY);

            RECT subtitleRect{34, 76, client.right - 32, 108};
            DrawTextW(
                hdc,
                L"Adicione aplicativos e deixe seus icones flutuando pela tela.",
                -1,
                &subtitleRect,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER
            );

            SelectObject(hdc, oldFont);
            DeleteObject(subtitleFont);

            RECT dropRect{32, 135, client.right - 32, client.bottom - 32};

            HPEN borderPen = CreatePen(PS_SOLID, 1, BORDER);
            HBRUSH dropBrush = CreateSolidBrush(SURFACE);
            HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
            HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, dropBrush));

            RoundRect(
                hdc,
                dropRect.left, dropRect.top,
                dropRect.right, dropRect.bottom,
                18, 18
            );

            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(dropBrush);
            DeleteObject(borderPen);

            HFONT dropFont = CreatePingoFont(18, FW_MEDIUM);
            oldFont = static_cast<HFONT>(SelectObject(hdc, dropFont));
            SetTextColor(hdc, TEXT_PRIMARY);

            RECT dropTextRect{
                dropRect.left + 20,
                (dropRect.top + dropRect.bottom) / 2 - 18,
                dropRect.right - 20,
                (dropRect.top + dropRect.bottom) / 2 + 18
            };

            DrawTextW(
                hdc,
                L"Solte um arquivo .exe aqui",
                -1,
                &dropTextRect,
                DT_CENTER | DT_SINGLELINE | DT_VCENTER
            );

            SelectObject(hdc, oldFont);
            DeleteObject(dropFont);

            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

    if (!RegisterClassW(&wc))
        return 0;

    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
