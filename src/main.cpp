#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <oleidl.h>
#include <shellapi.h>
#include <string>
#include <cwchar>

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
    constexpr COLORREF PENGUIN_WHITE = RGB(245, 245, 245);
    constexpr COLORREF PENGUIN_ORANGE = RGB(235, 150, 45);
    constexpr COLORREF FLOATING_TRANSPARENT = RGB(255, 0, 255);
    constexpr COLORREF FLOATING_TEXT_SHADOW = RGB(0, 0, 0);

    constexpr int WINDOW_WIDTH = 800;
    constexpr int WINDOW_HEIGHT = 500;
    constexpr int FLOATING_ICON_WIDTH = 96;
    constexpr int FLOATING_ICON_HEIGHT = 118;

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
    bool g_oleDropRegistered = false;
    PingoAppItem* g_currentItem = nullptr;
    HINSTANCE g_instance = nullptr;

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

    void DrawPingoMascot(HDC hdc, int centerX, int centerY)
    {
        // Simple native vector mascot: Pingo, the project's official penguin.
        HBRUSH black = CreateSolidBrush(RGB(5, 5, 5));
        HBRUSH white = CreateSolidBrush(PENGUIN_WHITE);
        HBRUSH orange = CreateSolidBrush(PENGUIN_ORANGE);

        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, black));
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, static_cast<HPEN>(GetStockObject(NULL_PEN))));

        Ellipse(hdc, centerX - 44, centerY - 55, centerX + 44, centerY + 55);

        SelectObject(hdc, white);
        Ellipse(hdc, centerX - 31, centerY - 4, centerX + 31, centerY + 49);

        SelectObject(hdc, black);
        Ellipse(hdc, centerX - 24, centerY - 39, centerX - 3, centerY - 18);
        Ellipse(hdc, centerX + 3, centerY - 39, centerX + 24, centerY - 18);

        SelectObject(hdc, white);
        Ellipse(hdc, centerX - 18, centerY - 34, centerX - 10, centerY - 26);
        Ellipse(hdc, centerX + 10, centerY - 34, centerX + 18, centerY - 26);

        SelectObject(hdc, orange);
        POINT beak[] = {
            {centerX - 9, centerY - 12},
            {centerX + 9, centerY - 12},
            {centerX, centerY - 2}
        };
        Polygon(hdc, beak, 3);

        SelectObject(hdc, orange);
        Ellipse(hdc, centerX - 31, centerY + 45, centerX - 5, centerY + 54);
        Ellipse(hdc, centerX + 5, centerY + 45, centerX + 31, centerY + 54);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(black);
        DeleteObject(white);
        DeleteObject(orange);
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

    void SetDropVisual(HWND hwnd, bool active)
    {
        if (g_isDragOver == active)
            return;

        g_isDragOver = active;
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    void DrawDropHint(HDC hdc, const RECT& dropRect, bool active)
    {
        const int centerX = (dropRect.left + dropRect.right) / 2;
        const int centerY = (dropRect.top + dropRect.bottom) / 2 - 18;

        HBRUSH circle = CreateSolidBrush(active ? PENGUIN_ORANGE : RGB(35, 35, 35));
        HPEN pen = CreatePen(PS_SOLID, 1, active ? PENGUIN_ORANGE : BORDER);

        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(hdc, circle));
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));

        Ellipse(hdc, centerX - 34, centerY - 34, centerX + 34, centerY + 34);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(circle);
        DeleteObject(pen);

        DrawTextLine(
            hdc,
            active ? L"Solte aqui!" : L"Arraste um .exe para começar",
            RECT{
                dropRect.left + 30,
                centerY + 48,
                dropRect.right - 30,
                centerY + 82
            },
            19,
            FW_MEDIUM,
            TEXT_PRIMARY,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );

        DrawTextLine(
            hdc,
            active
                ? L"O Pingo vai colocar o aplicativo na sua tela"
                : L"O Pingo pega o nome e o ícone automaticamente",
            RECT{
                dropRect.left + 30,
                centerY + 84,
                dropRect.right - 30,
                centerY + 116
            },
            14,
            FW_NORMAL,
            TEXT_SECONDARY,
            DT_CENTER | DT_SINGLELINE | DT_VCENTER
        );
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

        HBRUSH transparent = CreateSolidBrush(FLOATING_TRANSPARENT);
        FillRect(hdc, &client, transparent);
        DeleteObject(transparent);

        if (item->icon)
        {
            const int iconSize = item->size;
            const int x = (FLOATING_ICON_WIDTH - iconSize) / 2;
            const int y = 2;

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
            item->size + 8,
            FLOATING_ICON_WIDTH - 4,
            FLOATING_ICON_HEIGHT - 1
        };

        RECT shadowRect = textRect;
        OffsetRect(&shadowRect, 1, 1);

        DrawTextLine(
            hdc,
            item->name.c_str(),
            shadowRect,
            13,
            FW_NORMAL,
            FLOATING_TEXT_SHADOW,
            DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER
        );

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

    enum class PingoMenuCommand : UINT
    {
        Open = 1001,
        Rename,
        LocateFile,
        RunAsAdministrator,
        Remove,
        Configure
    };

    void OpenPingoItem(HWND hwnd, PingoAppItem* item)
    {
        if (!item || item->path.empty())
            return;

        ShellExecuteW(hwnd, L"open", item->path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    void LocatePingoItem(HWND hwnd, PingoAppItem* item)
    {
        if (!item || item->path.empty())
            return;

        std::wstring command = L"/select,\"" + item->path + L"\"";
        ShellExecuteW(hwnd, L"open", L"explorer.exe", command.c_str(), nullptr, SW_SHOWNORMAL);
    }

    void RunPingoItemAsAdministrator(HWND hwnd, PingoAppItem* item)
    {
        if (!item || item->path.empty())
            return;

        SetLastError(ERROR_SUCCESS);

        HINSTANCE result = ShellExecuteW(
            hwnd,
            L"runas",
            item->path.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL
        );

        if (reinterpret_cast<INT_PTR>(result) <= 32)
        {
            const DWORD error = GetLastError();

            if (error != ERROR_CANCELLED)
            {
                MessageBoxW(
                    hwnd,
                    L"Não foi possível executar o aplicativo como administrador.",
                    L"Pingo App",
                    MB_OK | MB_ICONERROR
                );
            }
        }
    }


    struct RenameDialogState
    {
        PingoAppItem* item = nullptr;
        HWND dialog = nullptr;
        HWND edit = nullptr;
        bool accepted = false;
    };

    constexpr int RENAME_EDIT = 2001;
    constexpr int RENAME_OK = 2002;
    constexpr int RENAME_CANCEL = 2003;

    LRESULT CALLBACK RenameDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* state = reinterpret_cast<RenameDialogState*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA)
        );

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

        case WM_CREATE:
        {
            state = reinterpret_cast<RenameDialogState*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA)
            );

            if (!state || !state->item)
                return -1;

            HFONT font = CreatePingoFont(16, FW_NORMAL);

            CreateWindowExW(
                0,
                L"STATIC",
                L"Novo nome",
                WS_CHILD | WS_VISIBLE,
                20, 18, 320, 26,
                hwnd,
                nullptr,
                g_instance,
                nullptr
            );

            state->edit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT",
                state->item->name.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                20, 50, 320, 34,
                hwnd,
                reinterpret_cast<HMENU>(RENAME_EDIT),
                g_instance,
                nullptr
            );

            HWND okButton = CreateWindowExW(
                0,
                L"BUTTON",
                L"Salvar",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                174, 98, 80, 32,
                hwnd,
                reinterpret_cast<HMENU>(RENAME_OK),
                g_instance,
                nullptr
            );

            HWND cancelButton = CreateWindowExW(
                0,
                L"BUTTON",
                L"Cancelar",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                260, 98, 80, 32,
                hwnd,
                reinterpret_cast<HMENU>(RENAME_CANCEL),
                g_instance,
                nullptr
            );

            SendMessageW(state->edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(okButton, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SendMessageW(cancelButton, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

            SetPropW(hwnd, L"PingoRenameFont", font);

            SetFocus(state->edit);
            SendMessageW(
                state->edit,
                EM_SETSEL,
                0,
                static_cast<LPARAM>(-1)
            );

            return 0;
        }

        case WM_COMMAND:
        {
            const int command = LOWORD(wParam);

            if (command == RENAME_OK)
            {
                if (state && state->edit && state->item)
                {
                    wchar_t buffer[512]{};
                    GetWindowTextW(state->edit, buffer, 512);

                    std::wstring newName = buffer;
                    if (!newName.empty())
                    {
                        state->item->name = newName;
                        state->accepted = true;
                    }
                }

                DestroyWindow(hwnd);
                return 0;
            }

            if (command == RENAME_CANCEL)
            {
                DestroyWindow(hwnd);
                return 0;
            }

            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
                return 0;
            }

            if (wParam == VK_RETURN)
            {
                SendMessageW(
                    hwnd,
                    WM_COMMAND,
                    MAKEWPARAM(RENAME_OK, BN_CLICKED),
                    0
                );
                return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_NCDESTROY:
        {
            HFONT font = static_cast<HFONT>(
                RemovePropW(hwnd, L"PingoRenameFont")
            );

            if (font)
                DeleteObject(font);

            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    bool RenamePingoItem(HWND owner, PingoAppItem* item)
    {
        if (!item)
            return false;

        static bool registered = false;

        constexpr wchar_t RENAME_CLASS[] = L"PingoRenameDialog";

        if (!registered)
        {
            WNDCLASSW wc{};
            wc.lpfnWndProc = RenameDialogProc;
            wc.hInstance = g_instance;
            wc.lpszClassName = RENAME_CLASS;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

            if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return false;

            registered = true;
        }

        RenameDialogState state{};
        state.item = item;

        RECT ownerRect{};
        GetWindowRect(owner, &ownerRect);

        const int width = 370;
        const int height = 165;
        const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
        const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

        HWND dialog = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            RENAME_CLASS,
            L\"Renomear aplicativo — Pingo App\",
            WS_CAPTION | WS_SYSMENU,
            x,
            y,
            width,
            height,
            owner,
            nullptr,
            g_instance,
            &state
        );

        if (!dialog)
            return false;

        state.dialog = dialog;

        EnableWindow(owner, FALSE);
        ShowWindow(dialog, SW_SHOW);
        UpdateWindow(dialog);

        MSG msg{};
        while (IsWindow(dialog) && GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            if (!IsDialogMessageW(dialog, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);

        if (state.accepted && item->floatingWindow)
            InvalidateRect(item->floatingWindow, nullptr, FALSE);

        return state.accepted;
    }

    void ShowPingoContextMenu(HWND hwnd, PingoAppItem* item, POINT screenPoint)
    {
        if (!item)
            return;

        HMENU menu = CreatePopupMenu();
        if (!menu)
            return;

        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::Open), L"Abrir");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::Rename), L"Renomear");
        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::LocateFile), L"Localizar arquivo");
        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::RunAsAdministrator), L"Executar como administrador");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::Configure), L"Configurar");
        AppendMenuW(menu, MF_STRING, static_cast<UINT>(PingoMenuCommand::Remove), L"Remover");

        SetForegroundWindow(hwnd);

        const UINT command = TrackPopupMenuEx(
            menu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON,
            screenPoint.x,
            screenPoint.y,
            hwnd,
            nullptr
        );

        DestroyMenu(menu);

        switch (static_cast<PingoMenuCommand>(command))
        {
        case PingoMenuCommand::Open:
            OpenPingoItem(hwnd, item);
            break;

        case PingoMenuCommand::LocateFile:
            LocatePingoItem(hwnd, item);
            break;

        case PingoMenuCommand::RunAsAdministrator:
            RunPingoItemAsAdministrator(hwnd, item);
            break;

        case PingoMenuCommand::Remove:
            DestroyAppItem(g_currentItem);
            break;

        case PingoMenuCommand::Rename:
            RenamePingoItem(hwnd, item);
            break;

        case PingoMenuCommand::Configure:
        default:
            break;
        }
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

        case WM_RBUTTONUP:
            if (item)
            {
                POINT point{};
                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);
                ClientToScreen(hwnd, &point);
                ShowPingoContextMenu(hwnd, item, point);
            }
            return 0;

        case WM_LBUTTONDBLCLK:
            if (item && !item->path.empty())
            {
                HINSTANCE result = ShellExecuteW(
                    hwnd,
                    L"open",
                    item->path.c_str(),
                    nullptr,
                    nullptr,
                    SW_SHOWNORMAL
                );

                if (reinterpret_cast<INT_PTR>(result) <= 32)
                {
                    // Keep the launcher silent on normal use; failed launches
                    // will be handled by the future context menu/error UI.
                }
            }
            return 0;

        case WM_EXITSIZEMOVE:
            if (item)
            {
                RECT windowRect{};
                if (GetWindowRect(hwnd, &windowRect))
                {
                    item->position.x = windowRect.left;
                    item->position.y = windowRect.top;
                }
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
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_LAYERED,
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

        SetLayeredWindowAttributes(
            floating,
            FLOATING_TRANSPARENT,
            0,
            LWA_COLORKEY
        );

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

    void HandleDroppedPath(HWND hwnd, HINSTANCE hInstance, const std::wstring& path)
    {
        if (!IsExeFile(path))
            return;

        DestroyAppItem(g_currentItem);
        g_currentItem = CreatePingoAppItem(hwnd, hInstance, path);
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    void HandleDroppedFile(HWND hwnd, HDROP drop, HINSTANCE hInstance)
    {
        const UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

        for (UINT i = 0; i < fileCount; ++i)
        {
            const UINT required = DragQueryFileW(drop, i, nullptr, 0);
            if (required == 0)
                continue;

            std::wstring path(required + 1, L'\0');
            if (DragQueryFileW(drop, i, path.data(), required + 1) == 0)
                continue;

            path.resize(required);
            HandleDroppedPath(hwnd, hInstance, path);

            if (g_currentItem)
                break;
        }

        DragFinish(drop);
    }

    class PingoDropTarget final : public IDropTarget
    {
    public:
        PingoDropTarget(HWND hwnd, HINSTANCE hInstance)
            : hwnd_(hwnd), hInstance_(hInstance)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid,
            void** ppvObject) override
        {
            if (!ppvObject)
                return E_POINTER;

            *ppvObject = nullptr;

            if (riid == IID_IUnknown || riid == IID_IDropTarget)
            {
                *ppvObject = static_cast<IDropTarget*>(this);
                AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return ++referenceCount_;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            const ULONG count = --referenceCount_;
            if (count == 0)
                delete this;
            return count;
        }

        HRESULT STDMETHODCALLTYPE DragEnter(
            IDataObject* dataObject,
            DWORD,
            POINTL,
            DWORD* effect) override
        {
            const bool accepts = HasExeFile(dataObject);
            SetDropVisual(hwnd_, accepts);

            if (effect)
                *effect = accepts ? DROPEFFECT_COPY : DROPEFFECT_NONE;

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE DragOver(
            DWORD,
            POINTL,
            DWORD* effect) override
        {
            if (effect)
                *effect = g_isDragOver ? DROPEFFECT_COPY : DROPEFFECT_NONE;

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE DragLeave() override
        {
            SetDropVisual(hwnd_, false);
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Drop(
            IDataObject* dataObject,
            DWORD,
            POINTL,
            DWORD* effect) override
        {
            bool handled = false;

            FORMATETC format{};
            format.cfFormat = CF_HDROP;
            format.ptd = nullptr;
            format.dwAspect = DVASPECT_CONTENT;
            format.lindex = -1;
            format.tymed = TYMED_HGLOBAL;

            STGMEDIUM medium{};
            if (SUCCEEDED(dataObject->GetData(&format, &medium)))
            {
                HDROP drop = static_cast<HDROP>(GlobalLock(medium.hGlobal));

                if (drop)
                {
                    const UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);

                    for (UINT i = 0; i < fileCount; ++i)
                    {
                        const UINT required = DragQueryFileW(drop, i, nullptr, 0);
                        if (required == 0)
                            continue;

                        std::wstring path(required + 1, L'\0');
                        if (DragQueryFileW(drop, i, path.data(), required + 1) != 0)
                        {
                            path.resize(required);

                            if (IsExeFile(path.c_str()))
                            {
                                HandleDroppedPath(hwnd_, hInstance_, path);
                                handled = true;
                                break;
                            }
                        }
                    }

                    GlobalUnlock(medium.hGlobal);
                }

                ReleaseStgMedium(&medium);
            }

            SetDropVisual(hwnd_, false);

            if (effect)
                *effect = handled ? DROPEFFECT_COPY : DROPEFFECT_NONE;

            return handled ? S_OK : S_FALSE;
        }

    private:
        static bool HasExeFile(IDataObject* dataObject)
        {
            if (!dataObject)
                return false;

            FORMATETC format{};
            format.cfFormat = CF_HDROP;
            format.ptd = nullptr;
            format.dwAspect = DVASPECT_CONTENT;
            format.lindex = -1;
            format.tymed = TYMED_HGLOBAL;

            return SUCCEEDED(dataObject->QueryGetData(&format));
        }

        ULONG referenceCount_ = 1;
        HWND hwnd_ = nullptr;
        HINSTANCE hInstance_ = nullptr;
    };

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_DROPFILES:
            if (!g_oleDropRegistered)
            {
                SetDropVisual(hwnd, false);
                HandleDroppedFile(hwnd, reinterpret_cast<HDROP>(wParam), g_instance);
            }
            return 0;

        case WM_DESTROY:
            if (g_oleDropRegistered)
            {
                RevokeDragDrop(hwnd);
                g_oleDropRegistered = false;
            }

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

            DrawPingoMascot(hdc, 82, 82);

            DrawTextLine(
                hdc,
                L"Pingo App",
                RECT{145, 34, client.right - 32, 78},
                30,
                FW_SEMIBOLD,
                TEXT_PRIMARY,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER
            );

            DrawTextLine(
                hdc,
                L"Seu pinguim para organizar aplicativos na tela.",
                RECT{147, 78, client.right - 32, 108},
                16,
                FW_NORMAL,
                TEXT_SECONDARY,
                DT_LEFT | DT_SINGLELINE | DT_VCENTER
            );

            RECT dropRect{32, 135, client.right - 32, client.bottom - 32};

            DrawRoundedPanel(
                hdc,
                dropRect,
                g_isDragOver ? RGB(30, 27, 20) : SURFACE,
                g_isDragOver ? PENGUIN_ORANGE : BORDER
            );

            DrawDropHint(hdc, dropRect, g_isDragOver);

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
    g_instance = hInstance;

    const HRESULT oleResult = OleInitialize(nullptr);
    if (FAILED(oleResult))
        return 0;

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc))
    {
        OleUninitialize();
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd)
    {
        OleUninitialize();
        return 0;
    }

    auto* dropTarget = new PingoDropTarget(hwnd, hInstance);
    if (SUCCEEDED(RegisterDragDrop(hwnd, dropTarget)))
    {
        g_oleDropRegistered = true;
        dropTarget->Release();
    }
    else
    {
        dropTarget->Release();
        DragAcceptFiles(hwnd, TRUE);
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    g_oleDropRegistered = false;
    OleUninitialize();

    return static_cast<int>(msg.wParam);
}
