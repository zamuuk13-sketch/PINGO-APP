#include <windows.h>
#include <shellapi.h>
#include <string>

namespace
{
    constexpr wchar_t WINDOW_CLASS[] = L"PingoAppWindow";
    constexpr wchar_t WINDOW_TITLE[] = L"Pingo App";
    constexpr wchar_t FLOATING_CLASS[] = L"PingoFloatingIcon";

    constexpr COLORREF BACKGROUND = RGB(13, 13, 13);
    constexpr COLORREF SURFACE = RGB(22, 22, 22);
    constexpr COLORREF SURFACE_HOVER = RGB(28, 28, 28);
    constexpr COLORREF BORDER = RGB(48, 48, 48);
    constexpr COLORREF BORDER_ACTIVE = RGB(80, 80, 80);
    constexpr COLORREF TEXT_PRIMARY = RGB(255, 255, 255);
    constexpr COLORREF TEXT_SECONDARY = RGB(165, 165, 165);

    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 500;
    constexpr int FLOATING_ICON_WIDTH = 96;
    constexpr int FLOATING_ICON_HEIGHT = 112;

    struct PingoAppItem
    {
        std::wstring path;
        std::wstring name;
        HICON icon = nullptr;
        POINT position{120, 180};
        int size = 72;
        HWND floatingWindow = nullptr;
    };

    bool g_isDragOver = false;
    PingoAppItem* g_currentItem = nullptr;

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

        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 18, 18);

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
        return extension && lstrcmpiW(extension, L".exe") == 0;
    }

    std::wstring GetFileNameWithoutExtension(const std::wstring& path)
    {
        const size_t slash = path.find_last_of(L"\\/");
        const size_t dot = path.find_last_of(L'.');

        const size_t nameStart = slash == std::wstring::npos ? 0 : slash + 1;
        const size_t nameEnd =
            dot != std::wstring::npos && dot > nameStart
                ? dot
                : path.length();

        return path.substr(nameStart, nameEnd - nameStart);
    }

    std::wstring GetExecutableName(const std::wstring& path)
    {
        DWORD dummy = 0;
        const DWORD versionSize = GetFileVersionInfoSizeW(path.c_str(), &dummy);

        if (versionSize > 0)
        {
            std::wstring buffer(versionSize, L'\\0');

            if (GetFileVersionInfoW(path.c_str(), 0, versionSize, buffer.data()))
            {
                struct LANGANDCODEPAGE
                {
                    WORD language;
                    WORD codePage;
                };

                LANGANDCODEPAGE* translations = nullptr;
                UINT translationSize = 0;

                if (VerQueryValueW(
                        buffer.data(),
                        L"\\VarFileInfo\\Translation",
                        reinterpret_cast<LPVOID*>(&translations),
                        &translationSize) &&
                    translationSize >= sizeof(LANGANDCODEPAGE))
                {
                    for (UINT i = 0;
                         i < translationSize / sizeof(LANGANDCODEPAGE);
                         ++i)
                    {
                        wchar_t subBlock[64]{};
                        wsprintfW(
                            subBlock,
                            L"\\StringFileInfo\\%04x%04x\\FileDescription",
                            translations[i].language,
                            translations[i].codePage
                        );

                        wchar_t* description = nullptr;
                        UINT descriptionLength = 0;

                        if (VerQueryValueW(
                                buffer.data(),
                                subBlock,
                                reinterpret_cast<LPVOID*>(&description),
                                &descriptionLength) &&
                            description &&
                            descriptionLength > 0)
                        {
                            return description;
                        }
                    }
                }
            }
        }

        return GetFileNameWithoutExtension(path);
    }

    HICON ExtractExecutableIcon(const std::wstring& path)
    {
        HICON largeIcon = nullptr;
        HICON smallIcon = nullptr;

        const UINT extracted = ExtractIconExW(
            path.c_str(),
            0,
            &largeIcon,
            &smallIcon,
            1
        );

        if (smallIcon)
            DestroyIcon(smallIcon);

        if (extracted == 0)
        {
            if (largeIcon)
                DestroyIcon(largeIcon);

            return nullptr;
        }

        return largeIcon;
    }

    void DrawFloatingIcon(HDC hdc, PingoAppItem* item)
    {
        RECT client{};
        GetClientRect(WindowFromDC(hdc), &client);

        HBRUSH background = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &client, background);
        DeleteObject(background);

        if (item->icon)
        {
            const int iconSize = item->size;
            const int x = (FLOATING_ICON_WIDTH - iconSize) / 2;
            const int y = 4;

            DrawIconEx(
                hdc,
                x,
                y,
                item->icon,
                iconSize,
                iconSize,
                0,
                nullptr,
                DI_NORMAL
            );
        }

        RECT textRect{
            4,
            item->size + 7,
            FLOATING_ICON_WIDTH - 4,
            FLOATING_ICON_HEIGHT - 2
        };

        DrawTextLine(
            hdc,
            item->name.c_str(),
            textRect,
            13,
            FW_NORMAL,
            TEXT_PRIMARY,
            DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER
        );
    }

    LRESULT CALLBACK FloatingWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        PingoAppItem* item =
            reinterpret_cast<PingoAppItem*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        switch (message)
        {
        case WM_NCCREATE:
        {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(create->lpCreateParams)
            );
            return TRUE;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);

            if (item)
                DrawFloatingIcon(hdc, item);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN:
            if (item)
            {
                ReleaseCapture();
                SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
            return 0;

        case WM_DESTROY:
            if (item)
                item->floatingWindow = nullptr;
            return 0;

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    bool RegisterFloatingWindowClass(HINSTANCE hInstance)
    {
        static bool registered = false;

        if (registered)
            return true;

        WNDCLASSW wc{};
        wc.lpfnWndProc = FloatingWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = FLOATING_CLASS;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.style = CS_HREDRAW | CS_VREDRAW;

        if (!RegisterClassW(&wc))
            return false;

        registered = true;
        return true;
    }

    bool CreateFloatingIconWindow(HWND owner, HINSTANCE hInstance, PingoAppItem* item)
    {
        if (!item || !RegisterFloatingWindowClass(hInstance))
            return false;

        HWND floating = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
            FLOATING_CLASS,
            item->name.c_str(),
            WS_POPUP,
            item->position.x,
            item->position.y,
            FLOATING_ICON_WIDTH,
            FLOATING_ICON_HEIGHT,
            owner,
            nullptr,
            hInstance,
            item
        );

        if (!floating)
            return false;

        item->floatingWindow = floating;

        ShowWindow(floating, SW_SHOWNOACTIVATE);
        UpdateWindow(floating);
        return true;
    }

    void DestroyAppItem(PingoAppItem*& item)
    {
        if (!item)
            return;

        if (item->floatingWindow)
        {
            DestroyWindow(item->floatingWindow);
            item->floatingWindow = nullptr;
        }

        if (item->icon)
            DestroyIcon(item->icon);

        delete item;
        item = nullptr;
    }

    PingoAppItem* CreatePingoAppItem(
        HWND owner,
        HINSTANCE hInstance,
        const std::wstring& path)
    {
        auto* item = new PingoAppItem;
        item->path = path;
        item->name = GetExecutableName(path);
        item->icon = ExtractExecutableIcon(path);

        if (!CreateFloatingIconWindow(owner, hInstance, item))
        {
            if (item->icon)
                DestroyIcon(item->icon);

            delete item;
            return nullptr;
        }

        return item;
    }

    void HandleDroppedFile(HWND hwnd, HDROP drop, HINSTANCE hInstance)
    {
        const UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

        for (UINT i = 0; i < fileCount; ++i)
        {
            wchar_t path[MAX_PATH]{};

            if (DragQueryFileW(drop, i, path, MAX_PATH) == 0)
                continue;

            if (!IsExeFile(path))
                continue;

            DestroyAppItem(g_currentItem);
            g_currentItem = CreatePingoAppItem(hwnd, hInstance, path);

            if (g_currentItem)
            {
                std::wstring message =
                    L"Icone flutuante criado para:\n" +
                    g_currentItem->name;

                MessageBoxW(
                    hwnd,
                    message.c_str(),
                    L"Pingo App",
                    MB_OK | MB_ICONINFORMATION
                );
            }

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
            HandleDroppedFile(
                hwnd,
                reinterpret_cast<HDROP>(wParam),
                reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE))
            );
            return 0;

        case WM_DESTROY:
            DestroyAppItem(g_currentItem);
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

            RECT dropRect{32, 135, client.right - 32, client.bottom - 32};

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

    SetWindowLongPtrW(hwnd, GWLP_HINSTANCE, reinterpret_cast<LONG_PTR>(hInstance));
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
