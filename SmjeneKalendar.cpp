// SmjeneKalendar.cpp
// Kompajliranje: g++ -o SmjeneKalendar.exe SmjeneKalendar.cpp -lgdi32 -lcomctl32 -mwindows -static

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <map>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ============================================================
// GLOBALNE VARIJABLE
// ============================================================
HINSTANCE g_hInst;
HWND g_hMainWnd;
HWND g_hMonthCombo, g_hYearCombo;
HWND g_hPrevBtn, g_hNextBtn;
HWND g_hStatsLabel;
HWND g_hTodayBtn;

// Boje
COLORREF CLR_NOCNA    = RGB(70, 70, 180);    
COLORREF CLR_JUTARNJA = RGB(255, 165, 0);    
COLORREF CLR_SLOBODAN = RGB(50, 180, 50);    
COLORREF CLR_HEADER   = RGB(40, 40, 40);     
COLORREF CLR_BG       = RGB(30, 30, 30);     
COLORREF CLR_CELL_BG  = RGB(50, 50, 50);     
COLORREF CLR_TODAY    = RGB(255, 215, 0);    
COLORREF CLR_TEXT     = RGB(255, 255, 255);  
COLORREF CLR_WEEKEND  = RGB(80, 60, 60);     

int g_currentMonth = 0; // 0-11
int g_currentYear = 2026;

struct RefDate {
    int day = 1;
    int month = 2; 
    int year = 2026;
    int cycleIndex = 0; 
};

RefDate g_ref;

enum SmjenaType {
    NOCNA_SMJENA = 0,
    SLOBODAN_1,
    SLOBODAN_2,
    JUTARNJA_SMJENA,
    NOCNA_SMJENA_2
};

const wchar_t* smjenaNames[] = {
    L"NOĆNA",
    L"SLOBODAN",
    L"SLOBODAN",
    L"JUTARNJA",
    L"NOĆNA"
};

const wchar_t* monthNames[] = {
    L"Januar", L"Februar", L"Mart", L"April",
    L"Maj", L"Juni", L"Juli", L"August",
    L"Septembar", L"Oktobar", L"Novembar", L"Decembar"
};

const wchar_t* dayHeaders[] = {
    L"PON", L"UTO", L"SRI", L"ČET", L"PET", L"SUB", L"NED"
};

#define ID_PREV_BTN   1001
#define ID_NEXT_BTN   1002
#define ID_MONTH_CMB  1003
#define ID_YEAR_CMB   1004
#define ID_TODAY_BTN  1005

// ============================================================
// POMOĆNE FUNKCIJE
// ============================================================

bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DaysInMonth(int month, int year) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && IsLeapYear(year)) return 29;
    return days[month - 1];
}

long long DaysBetween(int d1, int m1, int y1, int d2, int m2, int y2) {
    auto toJDN = [](int d, int m, int y) -> long long {
        long long a = (14 - m) / 12;
        long long yy = y + 4800 - a;
        long long mm = m + 12 * a - 3;
        return d + (153 * mm + 2) / 5 + 365 * yy + yy / 4 - yy / 100 + yy / 400 - 32045;
    };
    return toJDN(d1, m1, y1) - toJDN(d2, m2, y2);
}

SmjenaType GetSmjena(int day, int month, int year) {
    long long diff = DaysBetween(day, month, year, g_ref.day, g_ref.month, g_ref.year);
    int idx = (int)(((diff % 5) + 5) % 5); 
    idx = (idx + g_ref.cycleIndex) % 5;
    return (SmjenaType)idx;
}

// NOVA ISPRAVLJENA FUNKCIJA: 0=Ponedjeljak, 6=Nedjelja
int DayOfWeek(int d, int m, int y) {
    // Tomohiko Sakamoto algoritam
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int dow = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    // dow: 0=Ned, 1=Pon, 2=Uto, 3=Sri, 4=Cet, 5=Pet, 6=Sub
    // Pretvori u: 0=Pon, 1=Uto, 2=Sri, 3=Cet, 4=Pet, 5=Sub, 6=Ned
    if (dow == 0) return 6; // Nedjelja -> 6
    return dow - 1; // Pon=0, Uto=1, Sri=2, Cet=3, Pet=4, Sub=5
}

COLORREF GetSmjenaColor(SmjenaType s) {
    switch (s) {
        case NOCNA_SMJENA:
        case NOCNA_SMJENA_2:
            return CLR_NOCNA;
        case JUTARNJA_SMJENA:
            return CLR_JUTARNJA;
        case SLOBODAN_1:
        case SLOBODAN_2:
            return CLR_SLOBODAN;
    }
    return CLR_CELL_BG;
}

const wchar_t* GetSmjenaName(SmjenaType s) {
    return smjenaNames[(int)s];
}

bool IsToday(int d, int m, int y) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    return (d == st.wDay && m == st.wMonth && y == st.wYear);
}

// ============================================================
// CRTANJE
// ============================================================

void UpdateStats(HWND hwnd) {
    int month = g_currentMonth + 1;
    int year = g_currentYear;
    int daysInM = DaysInMonth(month, year);

    int nocnaCount = 0, jutarnjaCount = 0, slobodanCount = 0;
    int radniDani = 0;

    for (int d = 1; d <= daysInM; d++) {
        SmjenaType s = GetSmjena(d, month, year);
        switch (s) {
            case NOCNA_SMJENA:
            case NOCNA_SMJENA_2:
                nocnaCount++;
                radniDani++;
                break;
            case JUTARNJA_SMJENA:
                jutarnjaCount++;
                radniDani++;
                break;
            case SLOBODAN_1:
            case SLOBODAN_2:
                slobodanCount++;
                break;
        }
    }

    wchar_t buf[512];
    swprintf(buf, 512,
        L"Statistika za %s %d:  🔵Noćne:%d  🟠Jutarnje:%d  🟢Slobodni:%d  📋Radnih:%d",
        monthNames[g_currentMonth], year,
        nocnaCount, jutarnjaCount, slobodanCount, radniDani
    );
    SetWindowTextW(g_hStatsLabel, buf);
}

void DrawCalendar(HWND hwnd, HDC hdc) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

    HBRUSH bgBrush = CreateSolidBrush(CLR_BG);
    FillRect(memDC, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT hFontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT hFontBig = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT hFontSmall = CreateFontW(11, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

    SetBkMode(memDC, TRANSPARENT);

    int month = g_currentMonth + 1;
    int year = g_currentYear;

    int startX = 20;
    int startY = 80;
    int cellW = (width - 40) / 7;
    if (cellW < 50) cellW = 50;
    int cellH = 60;

    // Zaglavlje dana
    SelectObject(memDC, hFontBold);
    for (int i = 0; i < 7; i++) {
        RECT r;
        r.left = startX + i * cellW;
        r.top = startY;
        r.right = r.left + cellW;
        r.bottom = r.top + 25;

        HBRUSH hdrBrush = CreateSolidBrush(CLR_HEADER);
        FillRect(memDC, &r, hdrBrush);
        DeleteObject(hdrBrush);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);
        Rectangle(memDC, r.left, r.top, r.right, r.bottom);
        SelectObject(memDC, oldPen);
        DeleteObject(pen);

        SetTextColor(memDC, (i >= 5) ? RGB(255, 120, 120) : CLR_TEXT);
        DrawTextW(memDC, dayHeaders[i], -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    startY += 25;

    int firstDow = DayOfWeek(1, month, year); // 0=Pon, 6=Ned
    int daysInMonth = DaysInMonth(month, year);

    int row = 0;
    int col = firstDow;

    for (int day = 1; day <= daysInMonth; day++) {
        // Ako smo prešli u novi red
        if (col >= 7) {
            col = 0;
            row++;
        }

        SmjenaType smjena = GetSmjena(day, month, year);
        COLORREF smjColor = GetSmjenaColor(smjena);
        bool today = IsToday(day, month, year);
        bool weekend = (col >= 5); // Sub=5, Ned=6

        RECT cellRect;
        cellRect.left = startX + col * cellW;
        cellRect.top = startY + row * cellH;
        cellRect.right = cellRect.left + cellW;
        cellRect.bottom = cellRect.top + cellH;

        // Pozadina ćelije
        COLORREF bgColor = weekend ? CLR_WEEKEND : CLR_CELL_BG;
        HBRUSH cellBrush = CreateSolidBrush(bgColor);
        FillRect(memDC, &cellRect, cellBrush);
        DeleteObject(cellBrush);

        // Smjena traka
        RECT smjRect;
        smjRect.left = cellRect.left + 2;
        smjRect.top = cellRect.bottom - 20;
        smjRect.right = cellRect.right - 2;
        smjRect.bottom = cellRect.bottom - 2;

        HBRUSH smjBrush = CreateSolidBrush(smjColor);
        FillRect(memDC, &smjRect, smjBrush);
        DeleteObject(smjBrush);

        HPEN smjPen = CreatePen(PS_SOLID, 1, smjColor);
        HPEN oldP2 = (HPEN)SelectObject(memDC, smjPen);
        RoundRect(memDC, smjRect.left, smjRect.top, smjRect.right, smjRect.bottom, 4, 4);
        SelectObject(memDC, oldP2);
        DeleteObject(smjPen);

        // Tekst smjene
        SelectObject(memDC, hFontSmall);
        SetTextColor(memDC, RGB(255, 255, 255));
        DrawTextW(memDC, GetSmjenaName(smjena), -1, &smjRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Broj dana
        wchar_t dayStr[8];
        swprintf(dayStr, 8, L"%d", day);
        RECT dayRect = cellRect;
        dayRect.top += 3;
        dayRect.bottom = smjRect.top - 2;

        if (today) {
            int cx = (dayRect.left + dayRect.right) / 2;
            int cy = (dayRect.top + dayRect.bottom) / 2;
            HBRUSH todayBrush = CreateSolidBrush(CLR_TODAY);
            HPEN todayPen = CreatePen(PS_SOLID, 2, CLR_TODAY);
            HPEN oldP3 = (HPEN)SelectObject(memDC, todayPen);
            HBRUSH oldB3 = (HBRUSH)SelectObject(memDC, todayBrush);
            Ellipse(memDC, cx - 14, cy - 12, cx + 14, cy + 12);
            SelectObject(memDC, oldP3);
            SelectObject(memDC, oldB3);
            DeleteObject(todayBrush);
            DeleteObject(todayPen);

            SelectObject(memDC, hFontBig);
            SetTextColor(memDC, RGB(0, 0, 0));
        } else {
            SelectObject(memDC, hFontBig);
            SetTextColor(memDC, weekend ? RGB(255, 150, 150) : CLR_TEXT);
        }
        DrawTextW(memDC, dayStr, -1, &dayRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Okvir ćelije
        HPEN borderPen = CreatePen(PS_SOLID, 1, today ? CLR_TODAY : RGB(70, 70, 70));
        HPEN oldP4 = (HPEN)SelectObject(memDC, borderPen);
        HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldB4 = (HBRUSH)SelectObject(memDC, nullBrush);
        Rectangle(memDC, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom);
        SelectObject(memDC, oldP4);
        SelectObject(memDC, oldB4);
        DeleteObject(borderPen);

        col++;
    }

    // Legenda
    int legendY = startY + (row + 1) * cellH + 10;
    SelectObject(memDC, hFont);

    struct LegendItem {
        COLORREF color;
        const wchar_t* text;
    };
    LegendItem legends[] = {
        { CLR_NOCNA, L"  Noćna  " },
        { CLR_JUTARNJA, L"  Jutarnja  " },
        { CLR_SLOBODAN, L"  Slobodan  " },
        { CLR_TODAY, L"  Danas  " }
    };

    int legendX = startX;
    for (int i = 0; i < 4; i++) {
        RECT lr;
        lr.left = legendX;
        lr.top = legendY;
        lr.right = legendX + 16;
        lr.bottom = legendY + 16;

        HBRUSH lb = CreateSolidBrush(legends[i].color);
        HPEN lp = CreatePen(PS_SOLID, 1, legends[i].color);
        HPEN oldLP = (HPEN)SelectObject(memDC, lp);
        HBRUSH oldLB = (HBRUSH)SelectObject(memDC, lb);
        RoundRect(memDC, lr.left, lr.top, lr.right, lr.bottom, 3, 3);
        SelectObject(memDC, oldLP);
        SelectObject(memDC, oldLB);
        DeleteObject(lb);
        DeleteObject(lp);

        SetTextColor(memDC, CLR_TEXT);
        RECT tr;
        tr.left = legendX + 20;
        tr.top = legendY;
        tr.right = tr.left + 150;
        tr.bottom = legendY + 16;
        DrawTextW(memDC, legends[i].text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SIZE sz;
        GetTextExtentPoint32W(memDC, legends[i].text, (int)wcslen(legends[i].text), &sz);
        legendX += 20 + sz.cx + 15;
    }

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);

    DeleteObject(hFont);
    DeleteObject(hFontBold);
    DeleteObject(hFontBig);
    DeleteObject(hFontSmall);
}

void GoToToday() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    g_currentMonth = st.wMonth - 1;
    g_currentYear = st.wYear;

    SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);

    for (int i = 0; i < 21; i++) {
        if (2020 + i == g_currentYear) {
            SendMessageW(g_hYearCombo, CB_SETCURSEL, i, 0);
            break;
        }
    }
}

// ============================================================
// WINDOW PROCEDURE
// ============================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFontUI = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT hFontBoldUI = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

        g_hPrevBtn = CreateWindowW(L"BUTTON", L"◀ Prethodni",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            30, 15, 130, 35, hwnd, (HMENU)ID_PREV_BTN, g_hInst, NULL);
        SendMessageW(g_hPrevBtn, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        g_hMonthCombo = CreateWindowW(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            170, 18, 150, 300, hwnd, (HMENU)ID_MONTH_CMB, g_hInst, NULL);
        SendMessageW(g_hMonthCombo, WM_SETFONT, (WPARAM)hFontBoldUI, TRUE);
        for (int i = 0; i < 12; i++) {
            SendMessageW(g_hMonthCombo, CB_ADDSTRING, 0, (LPARAM)monthNames[i]);
        }
        SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);

        g_hYearCombo = CreateWindowW(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            330, 18, 90, 300, hwnd, (HMENU)ID_YEAR_CMB, g_hInst, NULL);
        SendMessageW(g_hYearCombo, WM_SETFONT, (WPARAM)hFontBoldUI, TRUE);
        for (int y = 2020; y <= 2040; y++) {
            wchar_t ystr[8];
            swprintf(ystr, 8, L"%d", y);
            SendMessageW(g_hYearCombo, CB_ADDSTRING, 0, (LPARAM)ystr);
        }
        SendMessageW(g_hYearCombo, CB_SETCURSEL, g_currentYear - 2020, 0);

        g_hNextBtn = CreateWindowW(L"BUTTON", L"Sljedeći ▶",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            430, 15, 130, 35, hwnd, (HMENU)ID_NEXT_BTN, g_hInst, NULL);
        SendMessageW(g_hNextBtn, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        g_hTodayBtn = CreateWindowW(L"BUTTON", L"📅 Danas",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            580, 15, 110, 35, hwnd, (HMENU)ID_TODAY_BTN, g_hInst, NULL);
        SendMessageW(g_hTodayBtn, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        g_hStatsLabel = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            30, 55, 750, 20, hwnd, NULL, g_hInst, NULL);
        SendMessageW(g_hStatsLabel, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        UpdateStats(hwnd);
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int notif = HIWORD(wParam);

        if (id == ID_PREV_BTN) {
            g_currentMonth--;
            if (g_currentMonth < 0) {
                g_currentMonth = 11;
                g_currentYear--;
            }
            SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);
            SendMessageW(g_hYearCombo, CB_SETCURSEL, g_currentYear - 2020, 0);
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == ID_NEXT_BTN) {
            g_currentMonth++;
            if (g_currentMonth > 11) {
                g_currentMonth = 0;
                g_currentYear++;
            }
            SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);
            SendMessageW(g_hYearCombo, CB_SETCURSEL, g_currentYear - 2020, 0);
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == ID_TODAY_BTN) {
            GoToToday();
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == ID_MONTH_CMB && notif == CBN_SELCHANGE) {
            g_currentMonth = (int)SendMessageW(g_hMonthCombo, CB_GETCURSEL, 0, 0);
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (id == ID_YEAR_CMB && notif == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(g_hYearCombo, CB_GETCURSEL, 0, 0);
            g_currentYear = 2020 + sel;
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawCalendar(hwnd, hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, CLR_TEXT);
        SetBkColor(hdcStatic, CLR_BG);
        static HBRUSH hBrushStatic = CreateSolidBrush(CLR_BG);
        return (LRESULT)hBrushStatic;
    }

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 800;
        mmi->ptMinTrackSize.y = 650;
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void VerifySchedule() {
    SmjenaType s1 = GetSmjena(1, 2, 2026);
    SmjenaType s2 = GetSmjena(2, 2, 2026);
    SmjenaType s3 = GetSmjena(3, 2, 2026);
    SmjenaType s4 = GetSmjena(4, 2, 2026);
    SmjenaType s5 = GetSmjena(5, 2, 2026);

    bool ok = (s1 == NOCNA_SMJENA || s1 == NOCNA_SMJENA_2) &&
              (s2 == SLOBODAN_1 || s2 == SLOBODAN_2) &&
              (s3 == SLOBODAN_1 || s3 == SLOBODAN_2) &&
              (s4 == JUTARNJA_SMJENA) &&
              (s5 == NOCNA_SMJENA || s5 == NOCNA_SMJENA_2);

    if (!ok) {
        MessageBoxW(NULL, L"GREŠKA: Raspored se ne poklapa sa zadanim!", L"Greška", MB_OK | MB_ICONERROR);
    }
}

// ============================================================
// MAIN
// ============================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;

    SYSTEMTIME st;
    GetLocalTime(&st);
    g_currentMonth = st.wMonth - 1;
    g_currentYear = st.wYear;

    VerifySchedule();

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BG);
    wc.lpszClassName = L"SmjeneKalendarClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(
        0, L"SmjeneKalendarClass",
        L"📅 Raspored Smjena - Kalendar",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 750,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
