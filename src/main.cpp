#include <windows.h>
#include <shellapi.h>

namespace
{
    constexpr wchar_t WINDOW_CLASS[] = L"PingoAppWindow";
    constexpr wchar_t WINDOW_TITLE[] = L"Pingo App";

    constexpr COLORREF BACKGROUND = RGB(13, 13, 13);
    constexpr COLORREF SURFACE = RGB(22, 22, 22);
    constexpr COLORREF SURFACE_HOVER = RGB(28, 28, 28);
    constexpr COLORREF BORDER = RGB(48, 48, 48);
    constexpr COLORREF BORDER_ACTIVE = RGB(80, 80, 80);
    constexpr COLORREF TEXT_PRIMARY = RGB(255, 255, 255);
    constexpr COLORREF TEXT_SECONDARY = RGB(165, 165, 165);

    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 500;

    bool g_isDragOver = false;

    HFONT CreatePingoFont(int size, int weight)
    {
        return CreateFontW(
            size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI Variable"
        );
    }

    void DrawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border)
    {
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HBRUSH brush = CreateSolidBrush(fill);

        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, brush));

        RoundRect(
            hdc,
            rect.left, rect.top,
            rect.right, rect.bottom,
            18, 18
        );

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void DrawTextLine(
        HDC hdc,
        const wchar_t* text,
        const RECT& rect,
        int fontSize,
        int fontWeight,
        COLORREF color,
        UINT format)
    {
        HFONT font = CreatePingoFont(fontSize, fontWeight);
        HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        DrawTextW(hdc, text, -1, const_cast<RECT*>(&rect), format);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    bool IsExeFile(const wchar_t* path)
    {
        const wchar_t* extension = wcsrchr(path, L'.');
        if (!extension)
            return false;

        return lstrcmpiW(extension, L".exe") == 0;
    }

    void HandleDroppedFile(HWND hwnd, HDROP drop)
    {
        UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

        for (UINT i = 0; i < fileCount; ++i)
        {
            wchar_t path[MAX_PATH]{};

            if (DragQueryFileW(drop, i, path, MAX_PATH) == 0)
                continue;

            if (!IsExeFile(path))
                continue;

            // Etapa 4 apenas recebe e valida o .exe.
            // A leitura do nome e do icone sera implementada na Etapa 5/6.
            MessageBoxW(
                hwnd,
                path,
                L"Aplicativo recebido",
                MB_OK | MB_ICONINFORMATION
            );

            break;
        }

        DragFinish(drop);
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_MOUSEMOVE:
            return 0;

        case WM_DROPFILES:
            g_isDragOver = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            HandleDroppedFile(hwnd, reinterpret_cast<HDROP>(wParam));
            return 0;

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

            DrawTextLine(
                hdc,
                L"Pingo App",
                RECT{32, 28, client.right - 32, 72},
                30,
                FW_SEMIBOLD,
                TEXT_PRIMARY,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER
            );

            DrawTextLine(
                hdc,
                L"Adicione aplicativos e deixe seus icones flutuando pela tela.",
                RECT{34, 76, client.right - 32, 108},
                16,
                FW_NORMAL,
                TEXT_SECONDARY,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER
            );

            RECT dropRect{
                32,
                135,
                client.right - 32,
                client.bottom - 32
            };

            DrawRoundedPanel(
                hdc,
                dropRect,
                g_isDragOver ? SURFACE_HOVER : SURFACE,
                g_isDragOver ? BORDER_ACTIVE : BORDER
            );

            DrawTextLine(
                hdc,
                g_isDragOver
                    ? L"Solte o arquivo .exe aqui"
                    : L"Arraste um arquivo .exe para ca",
                RECT{
                    dropRect.left + 20,
                    (dropRect.top + dropRect.bottom) / 2 - 18,
                    dropRect.right - 20,
                    (dropRect.top + dropRect.bottom) / 2 + 18
                },
                18,
                FW_MEDIUM,
                TEXT_PRIMARY,
                DT_CENTER | DT_SINGLELINE | DT_VCENTER
            );

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
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc))
        return 0;

    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd)
        return 0;

    DragAcceptFiles(hwnd, TRUE);

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
