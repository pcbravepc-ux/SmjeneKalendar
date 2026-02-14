#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// MAGICNI PRAGOVI - OVO DAJE MODERNI IZGLED I ISPRAVAN DPI
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(linker, "/subsystem:windows,5.02")
#pragma comment(linker, "/highdpiaware")

#include <windows.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ============================================================
// POSTAVKE - OVDJE MIJENJAJ AKO TREBA
// ============================================================

// Boje
COLORREF CLR_NOCNA    = RGB(70, 70, 180);    // Plava - noćna
COLORREF CLR_JUTARNJA = RGB(255, 165, 0);    // Narandžasta - jutarnja
COLORREF CLR_SLOBODAN = RGB(50, 180, 50);    // Zelena - slobodan
COLORREF CLR_HEADER   = RGB(40, 40, 40);     // Tamna - header
COLORREF CLR_BG       = RGB(30, 30, 30);     // Pozadina
COLORREF CLR_CELL_BG  = RGB(50, 50, 50);     // Ćelija pozadina
COLORREF CLR_TODAY     = RGB(255, 215, 0);   // Zlatna - danas
COLORREF CLR_TEXT     = RGB(255, 255, 255);   // Bijeli tekst
COLORREF CLR_WEEKEND  = RGB(80, 60, 60);     // Vikend pozadina

// Referentna tačka: 01.02.2026 = Noćna
// AKO TI SE CIKLUS PROMIJENI SAMO OVDJE MIJENJAJ
struct RefDate {
    int day = 1;
    int month = 2; // 1-12
    int year = 2026;
    int cycleIndex = 0; // 0 = Noćna
};

RefDate g_ref;

// ============================================================
// GLOBALNE VARIJABLE
// ============================================================
HINSTANCE g_hInst;
HWND g_hMainWnd;
HWND g_hPrevBtn, g_hNextBtn;
HWND g_hMonthCombo, g_hYearCombo;
HWND g_hStatsLabel;
HWND g_hTodayBtn;

int g_currentMonth = 0;
int g_currentYear = 2026;

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
    int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
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

int DayOfWeek(int day, int month, int year) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return ((h + 5) % 7);
}

COLORREF GetSmjenaColor(SmjenaType s) {
    switch (s) {
        case NOCNA_SMJENA: case NOCNA_SMJENA_2: return CLR_NOCNA;
        case JUTARNJA_SMJENA: return CLR_JUTARNJA;
        default: return CLR_SLOBODAN;
    }
}

const wchar_t* GetSmjenaName(SmjenaType s) {
    return smjenaNames[(int)s];
}

bool IsToday(int d, int m, int y) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    return (d == st.wDay && m == st.wMonth && y == st.wYear);
}

void CenterWindow(HWND hwnd) {
    RECT rc, sc;
    GetWindowRect(hwnd, &rc);
    GetWindowRect(GetDesktopWindow(), &sc);
    int x = (sc.right - rc.right) / 2;
    int y = (sc.bottom - rc.bottom) / 2;
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

// ============================================================
void UpdateStats(HWND hwnd) {
    int month = g_currentMonth + 1;
    int year = g_currentYear;
    int daysInM = DaysInMonth(month, year);

    int nocna = 0, jutarnja = 0, slobodan = 0;

    for (int d = 1; d <= daysInM; d++) {
        SmjenaType s = GetSmjena(d, month, year);
        switch (s) {
            case NOCNA_SMJENA: case NOCNA_SMJENA_2: nocna++; break;
            case JUTARNJA_SMJENA: jutarnja++; break;
            default: slobodan++; break;
        }
    }

    wchar_t buf[512];
    swprintf(buf, 512,
        L"📊 Statistika: 🔵 Noćne: %d   🟠 Jutarnje: %d   🟢 Slobodni: %d   📋 Ukupno radnih: %d",
        nocna, jutarnja, slobodan, nocna + jutarnja
    );
    SetWindowTextW(g_hStatsLabel, buf);
}

// ============================================================
// CRTANJE KALENDARA
// ============================================================

void DrawCalendar(HWND hwnd, HDC hdc) {
    RECT client;
    GetClientRect(hwnd, &client);
    int w = client.right;
    int h = client.bottom;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    HBRUSH bgBrush = CreateSolidBrush(CLR_BG);
    FillRect(memDC, &client, bgBrush);
    DeleteObject(bgBrush);

    HFONT hFont = CreateFontW(16, 0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HFONT hFontBold = CreateFontW(16,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HFONT hFontBig = CreateFontW(22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    HFONT hFontSmall = CreateFontW(12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");

    SetBkMode(memDC, TRANSPARENT);

    int month = g_currentMonth + 1;
    int year = g_currentYear;

    int startX = 30;
    int startY = 80;
    int cellW = (w - 60) / 7;
    int cellH = 70;

    // Header dana
    SelectObject(memDC, hFontBold);
    for (int i = 0; i < 7; i++) {
        RECT r = {startX + i*cellW, startY, startX + (i+1)*cellW, startY+30};
        HBRUSH br = CreateSolidBrush(CLR_HEADER);
        FillRect(memDC, &r, br);
        DeleteObject(br);

        SetTextColor(memDC, i >=5 ? RGB(255,100,100) : CLR_TEXT);
        DrawTextW(memDC, dayHeaders[i], -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        HPEN p = CreatePen(PS_SOLID, 1, RGB(80,80,80));
        SelectObject(memDC, p);
        Rectangle(memDC, r.left, r.top, r.right, r.bottom);
        DeleteObject(p);
    }

    startY += 30;

    int firstDow = DayOfWeek(1, month, year);
    int daysInM = DaysInMonth(month, year);
    int col = firstDow;
    int row = 0;

    for (int day = 1; day <= daysInM; day++) {
        SmjenaType s = GetSmjena(day, month, year);
        bool today = IsToday(day, month, year);
        bool weekend = col >=5;

        RECT r = {startX + col*cellW, startY + row*cellH, startX + (col+1)*cellW, startY + (row+1)*cellH};

        // Pozadina
        HBRUSH br = CreateSolidBrush(weekend ? CLR_WEEKEND : CLR_CELL_BG);
        FillRect(memDC, &r, br);
        DeleteObject(br);

        // Smjena
        RECT sr = {r.left + 4, r.bottom - 23, r.right -4, r.bottom -4};
        HBRUSH sb = CreateSolidBrush(GetSmjenaColor(s));
        HPEN sp = CreatePen(PS_SOLID, 1, GetSmjenaColor(s));
        SelectObject(memDC, sb);
        SelectObject(memDC, sp);
        RoundRect(memDC, sr.left, sr.top, sr.right, sr.bottom, 6,6);
        DeleteObject(sb);
        DeleteObject(sp);

        SelectObject(memDC, hFontSmall);
        SetTextColor(memDC, CLR_TEXT);
        DrawTextW(memDC, GetSmjenaName(s), -1, &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Broj dana
        wchar_t dstr[8];
        swprintf(dstr, 8, L"%d", day);
        RECT dr = r;
        dr.bottom = sr.top;

        if (today) {
            int cx = (dr.left + dr.right) / 2;
            int cy = (dr.top + dr.bottom) / 2;
            HBRUSH tb = CreateSolidBrush(CLR_TODAY);
            SelectObject(memDC, tb);
            Ellipse(memDC, cx-15, cy-13, cx+15, cy+13);
            DeleteObject(tb);
            SetTextColor(memDC, RGB(0,0,0));
        } else {
            SetTextColor(memDC, weekend ? RGB(255,150,150) : CLR_TEXT);
        }

        SelectObject(memDC, hFontBig);
        DrawTextW(memDC, dstr, -1, &dr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Okvir
        HPEN bp = CreatePen(PS_SOLID, today ? 3 : 1, today ? CLR_TODAY : RGB(70,70,70));
        SelectObject(memDC, bp);
        SelectObject(memDC, GetStockObject(NULL_BRUSH));
        Rectangle(memDC, r.left, r.top, r.right, r.bottom);
        DeleteObject(bp);

        col++;
        if (col ==7) { col=0; row++; }
    }

    // Legenda
    int ly = startY + (row + (col>0?1:0)) * cellH + 20;
    SelectObject(memDC, hFontBold);
    SetTextColor(memDC, CLR_TEXT);

    struct Legend { COLORREF c; const wchar_t* t; };
    Legend leg[] = {{CLR_NOCNA, L"Noćna"}, {CLR_JUTARNJA, L"Jutarnja"}, {CLR_SLOBODAN, L"Slobodan"}, {CLR_TODAY, L"Danas"}};

    int lx = startX;
    for(int i=0; i<4; i++) {
        RECT lr = {lx, ly, lx+18, ly+18};
        HBRUSH lb = CreateSolidBrush(leg[i].c);
        SelectObject(memDC, lb);
        RoundRect(memDC, lr.left, lr.top, lr.right, lr.bottom, 4,4);
        DeleteObject(lb);

        RECT tr = {lx+25, ly, lx+200, ly+18};
        DrawTextW(memDC, leg[i].t, -1, &tr, DT_VCENTER | DT_SINGLELINE);
        lx += 120;
    }

    BitBlt(hdc, 0,0,w,h, memDC, 0,0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    DeleteObject(hFont);
    DeleteObject(hFontBold);
    DeleteObject(hFontBig);
    DeleteObject(hFontSmall);
}

void GoToToday() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    g_currentMonth = st.wMonth -1;
    g_currentYear = st.wYear;
    SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);
    SendMessageW(g_hYearCombo, CB_SETCURSEL, g_currentYear - 2020, 0);
}

// ============================================================
// WND PROC
// ============================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {

        case WM_CREATE: {
            HFONT hf = CreateFontW(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
            HFONT hfb = CreateFontW(18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");

            g_hPrevBtn = CreateWindowW(L"BUTTON", L"◀ Prethodni", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 30,15,130,35, hwnd, (HMENU)ID_PREV_BTN, g_hInst, NULL);
            g_hMonthCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 170,18,150,300, hwnd, (HMENU)ID_MONTH_CMB, g_hInst, NULL);
            g_hYearCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 330,18,90,300, hwnd, (HMENU)ID_YEAR_CMB, g_hInst, NULL);
            g_hNextBtn = CreateWindowW(L"BUTTON", L"Sljedeći ▶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 430,15,130,35, hwnd, (HMENU)ID_NEXT_BTN, g_hInst, NULL);
            g_hTodayBtn = CreateWindowW(L"BUTTON", L"📅 Danas", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 580,15,110,35, hwnd, (HMENU)ID_TODAY_BTN, g_hInst, NULL);
            g_hStatsLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 30,55, 750,20, hwnd, NULL, g_hInst, NULL);

            SendMessageW(g_hPrevBtn, WM_SETFONT, (WPARAM)hf, TRUE);
            SendMessageW(g_hNextBtn, WM_SETFONT, (WPARAM)hf, TRUE);
            SendMessageW(g_hTodayBtn, WM_SETFONT, (WPARAM)hf, TRUE);
            SendMessageW(g_hMonthCombo, WM_SETFONT, (WPARAM)hfb, TRUE);
            SendMessageW(g_hYearCombo, WM_SETFONT, (WPARAM)hfb, TRUE);
            SendMessageW(g_hStatsLabel, WM_SETFONT, (WPARAM)hf, TRUE);

            for(int i=0; i<12; i++) SendMessageW(g_hMonthCombo, CB_ADDSTRING, 0, (LPARAM)monthNames[i]);
            for(int y=2020; y<=2045; y++) { wchar_t ys[8]; swprintf(ys,8,L"%d",y); SendMessageW(g_hYearCombo, CB_ADDSTRING,0,(LPARAM)ys); }

            GoToToday();
            UpdateStats(hwnd);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int nf = HIWORD(wParam);

            if(id == ID_PREV_BTN) {
                g_currentMonth--;
                if(g_currentMonth <0) { g_currentMonth=11; g_currentYear--; }
            }
            if(id == ID_NEXT_BTN) {
                g_currentMonth++;
                if(g_currentMonth>11) { g_currentMonth=0; g_currentYear++; }
            }
            if(id == ID_TODAY_BTN) GoToToday();
            if(id == ID_MONTH_CMB && nf == CBN_SELCHANGE) g_currentMonth = SendMessageW(g_hMonthCombo, CB_GETCURSEL,0,0);
            if(id == ID_YEAR_CMB && nf == CBN_SELCHANGE) g_currentYear = 2020 + SendMessageW(g_hYearCombo, CB_GETCURSEL,0,0);

            SendMessageW(g_hMonthCombo, CB_SETCURSEL, g_currentMonth, 0);
            SendMessageW(g_hYearCombo, CB_SETCURSEL, g_currentYear - 2020, 0);
            UpdateStats(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_KEYDOWN:
            if(wParam == VK_ESCAPE) DestroyWindow(hwnd);
            if(wParam == VK_LEFT) SendMessage(hwnd, WM_COMMAND, ID_PREV_BTN, 0);
            if(wParam == VK_RIGHT) SendMessage(hwnd, WM_COMMAND, ID_NEXT_BTN, 0);
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawCalendar(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_ERASEBKGND: return 1;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, CLR_TEXT);
            SetBkColor(hdc, CLR_BG);
            static HBRUSH hbr = CreateSolidBrush(CLR_BG);
            return (LRESULT)hbr;
        }

        case WM_DESTROY: PostQuitMessage(0); break;

        default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ============================================================
// MAIN
// ============================================================

void VerifySchedule() {
    if(GetSmjena(1,2,2026) != NOCNA_SMJENA) {
        MessageBoxW(NULL, L"Greška u izračunu rasporeda!", L"Greška", MB_ICONERROR);
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    g_hInst = hInstance;

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
    wc.lpszClassName = L"SmjeneKalendar";

    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(
        0, L"SmjeneKalendar",
        L"📅 Kalendar Smjena",
        WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,
        850, 700,
        NULL, NULL, hInstance, NULL
    );

    CenterWindow(g_hMainWnd);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while(GetMessageW(&msg, NULL, 0,0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
