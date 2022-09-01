// AutoScan.cpp : Defines the entry point for the application.
//

#include "AutoScan.h"
#include <atomic>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <tchar.h>
#include <psapi.h>

#define TimerPeriod 5000
#define TimerDelay 50

using namespace std;
static HHOOK kbdHook = NULL;
static set<DWORD> _keys;
static atomic<bool> _running = false;
static atomic<HWND> _eveWindow;
static std::string windowName("EVE - ");
static std::string character("YOUR_CHAR_NAME"); //change this
static BYTE targetKey(VK_SPACE); //change this

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        KBDLLHOOKSTRUCT kbd = *((KBDLLHOOKSTRUCT*)lParam);
        // the action is valid: HC_ACTION.f
        if (wParam == WM_KEYDOWN)
        {
            // lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
            _keys.insert(kbd.vkCode);
            cout << ":" << kbd.vkCode;
        }

        if (wParam == WM_KEYUP)
        {
            _keys.erase(kbd.vkCode);
        }
    }

    cout << _keys.size() << endl;


    fflush(stdout);
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    MSG msg;

    kbdHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

DWORD WINAPI TimerProc(LPVOID lpParameter)
{
    while (_running)
    {
        if (_keys.empty())
        {
            HWND currentActiveWindow = GetForegroundWindow();
            char* currentActiveWindowTitle = GetWindowTitle(currentActiveWindow);
            if (strstr(currentActiveWindowTitle, windowName.c_str()))
            {
                PressKey(targetKey);
            }
            else
            {
                if (_eveWindow != 0)
                {
                    SetForegroundWindow(_eveWindow);
                    PressKey(targetKey); // 
                }

                if (currentActiveWindow != 0)
                {
                    SetForegroundWindow(currentActiveWindow);
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(TimerPeriod));
            _keys.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(TimerPeriod));
    }

    return 0;
}

void PressKey(BYTE key)
{
    // Simulate a key press
    keybd_event(key, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(TimerDelay));
    // Simulate a key release
    keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
}

char* GetWindowTitle(HWND hWnd)
{
    int length = GetWindowTextLength(hWnd);
    char* buffer = new char[length + 1];
    GetWindowText(hWnd, buffer, length + 1);
    return buffer;
}


BOOL CALLBACK fnEnumWindowProc(HWND hWnd, LPARAM lParam) {
    char* title = GetWindowTitle(hWnd);

    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);

    if (strstr(title, windowName.c_str()))
    {
        if (strstr(title, character.c_str()))
        {
            _eveWindow = hWnd;
        }

        cout << hWnd << ":" << pid << ":" << title << endl;

    }
    return TRUE;
}


int main() {
    std::string str;
    EnumWindows(fnEnumWindowProc, 0);

    _running = true;
    CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
    CreateThread(NULL, 0, TimerProc, NULL, 0, NULL);

    std::getline(std::cin, str);
    _running = false;

    UnhookWindowsHookEx(kbdHook);

    cout << "Disposing" << endl;

    return 0;
}