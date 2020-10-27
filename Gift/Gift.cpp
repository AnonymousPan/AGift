// Gift.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Gift.h"
#include "Utils.h"
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <combaseapi.h>

#pragma warning(disable : 4996)
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;
using namespace Gdiplus;

//////////////////////////////
//////// Begin Config ////////
//////////////////////////////

// Set this to TRUE to calculate and show the hash of sound file and image file.
// This will also disable file hash check
#define DBG_GET_HASH FALSE

// Enable keyboard lock?
// This will block any hotkeys except Ctrl+Alt+Del
#define ENABLE_KEYBOARD_LOCK TRUE

// Run in fullscreen mode?
#define FULLSCREEN_MODE TRUE

// Check the CRC32 of the sound file on starting?
#define ENABLE_SOUND_FILE_HASH_CHECK TRUE
// The CRC32 of the sound file
#define SOUND_FILE_HASH 0x4690688E

// Check the CRC32 of the image file on starting?
#define ENABLE_IMAGE_FILE_HASH_CHECK TRUE
// The CRC32 of the image file
#define IMAGE_FILE_HASH 0xD321E827

// The interval between pressing Volume-Up key (in millseconds)
// Set this to 0 to disable auto volume up
#define VOLUME_UP_INTERVAL 10

// Enable anti-exit?
// When user click on the exit button on the top of the window,
// the program will not exit and show a Message Box.
#define ENABLE_ANTI_EXIT TRUE
// The text of the Message Box
#define TEXT_ON_EXIT L"You can't close me :D"

//////////////////////////////
///////// End Config /////////
//////////////////////////////

#define VOLUME_UP_TIMER_ID 1000

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // hInst
HWND thisWindow;                                // The handle of this window
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HHOOK kbHook;                                   // Keyboard Hook
Bitmap *bgImage;                                // Background Image

// GDI
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    KeyboardProc(int, WPARAM, LPARAM);
VOID CALLBACK       VolUpTimerProc(HWND, UINT, UINT_PTR, DWORD);
VOID                Fullscreen();
VOID                OnKeyboardLockError(DWORD);
VOID                OnImageLoadError(DWORD);
VOID                OnSoundLoadError(DWORD);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GIFT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    // GDI Init
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Keyboard Lock Init
    if (ENABLE_KEYBOARD_LOCK)
    {
        kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, NULL, 0);
        if (kbHook == NULL) OnKeyboardLockError(GetLastError());
    }

    // Auto Volume Up Init
    if (VOLUME_UP_INTERVAL > 0)
    {
        SetTimer(thisWindow, VOLUME_UP_TIMER_ID, VOLUME_UP_INTERVAL, (TIMERPROC)VolUpTimerProc);
    }

    WCHAR szDebugMessage[200] = L"Image file CRC32 = 0x";

    // Load Background Image
    HRSRC hBgImgRes = FindResource(hInst, MAKEINTRESOURCE(IDB_IMAGE), RT_BITMAP);
    DWORD dwBgImgSize = SizeofResource(hInst, hBgImgRes);
    HGLOBAL hBgImgGlobal = LoadResource(hInst, hBgImgRes);
    LPVOID pBgImg = LockResource(hBgImgGlobal);
    if (pBgImg == NULL) OnImageLoadError(GetLastError());
    
    if (ENABLE_IMAGE_FILE_HASH_CHECK | DBG_GET_HASH)
    {
        UINT bgImgCRC = GetCRC32((unsigned char*)pBgImg, dwBgImgSize);
        if (bgImgCRC != IMAGE_FILE_HASH && DBG_GET_HASH == FALSE) OnImageLoadError(0L);
        if (DBG_GET_HASH)
        {
            WCHAR mHash[8] = L"";
            _itow(bgImgCRC, mHash, 16);
            wcscat(szDebugMessage, mHash);
            wcscat(szDebugMessage, L"\nSound file CRC32 = 0x");
        }
    }

    HBITMAP hBgImg = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_IMAGE));
    bgImage = Bitmap::FromHBITMAP(hBgImg, NULL);

    // Load Sound
    HRSRC hSoundRes = FindResource(hInst, MAKEINTRESOURCE(IDR_WAVE), L"WAVE");
    DWORD aaa = GetLastError();
    DWORD dwSoundSize = SizeofResource(hInst, hSoundRes);
    HGLOBAL hSoundGlobal = LoadResource(hInst, hSoundRes);
    LPVOID pSound = LockResource(hSoundGlobal);
    if (pSound == NULL) OnSoundLoadError(GetLastError());

    if (ENABLE_SOUND_FILE_HASH_CHECK | DBG_GET_HASH)
    {
        UINT soundCRC = GetCRC32((unsigned char*)pSound, dwSoundSize);
        if (soundCRC != SOUND_FILE_HASH && DBG_GET_HASH == FALSE) OnSoundLoadError(0L);
        if (DBG_GET_HASH)
        {
            WCHAR mHash[8] = L"";
            _itow(soundCRC, mHash, 16);
            wcscat(szDebugMessage, mHash);
            MessageBox(thisWindow, szDebugMessage, L"Debug Information", 0);
        }
    }

    // Start playing sound
    PlaySound(MAKEINTRESOURCE(IDR_WAVE), hInst, SND_RESOURCE | SND_ASYNC | SND_LOOP);

    // Fullscreen
    if (FULLSCREEN_MODE)
    {
        RECT screenRect;
        screenRect.left = 0;
        screenRect.top = 0;
        UINT w = GetSystemMetrics(SM_CXSCREEN);
        UINT h = GetSystemMetrics(SM_CYSCREEN);
        screenRect.right = w;
        screenRect.bottom = h;
        SetWindowLong(thisWindow, GWL_STYLE, GetWindowLong(thisWindow, GWL_STYLE) & ~WS_CAPTION);
        MoveWindow(thisWindow, -10, -10, w + 20, h + 20, TRUE);
        SetWindowPos(thisWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(thisWindow, VOLUME_UP_TIMER_ID);
    GdiplusShutdown(gdiplusToken);
    return (int) msg.wParam;
}

////////////////////////////////////////////////////////
// 键盘钩子回调
////////////////////////////////////////////////////////
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* kblp = (KBDLLHOOKSTRUCT*)lParam;
    if (kblp->vkCode != VK_VOLUME_UP)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GIFT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_NO);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GIFT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
   thisWindow = hWnd;

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        {
            RECT wndRect;
            GetWindowRect(thisWindow, &wndRect);
            UINT w = wndRect.right - wndRect.left;
            UINT h = wndRect.bottom - wndRect.top;
            
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            Graphics g(hdc);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(bgImage, 0, 0, w, h);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        if (ENABLE_ANTI_EXIT)
        {
            MessageBox(thisWindow, TEXT_ON_EXIT, L"NO!", MB_OK | MB_ICONERROR);
            return 1;
        }
        else
        {
            PostQuitMessage(0);
            break;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

VOID CALLBACK VolUpTimerProc(HWND hWnd, UINT message, UINT_PTR timerId, DWORD ticks)
{
    SetVolume(100);
}

VOID OnKeyboardLockError(DWORD code)
{
    PostQuitMessage(114514);
}

VOID OnImageLoadError(DWORD code)
{
    PostQuitMessage(114514);
}

VOID OnSoundLoadError(DWORD code)
{
    PostQuitMessage(114514);
}