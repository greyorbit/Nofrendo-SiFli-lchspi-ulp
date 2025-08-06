/**
 * Copyright (c) 2025 CJS<greyorbit@foxmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <windows.h>
#include <stdint.h>
#include <stdio.h>

enum {
    ENUM_JOYPAD_A,      // event_joypad1_a,
    ENUM_JOYPAD_B,      // event_joypad1_b,
    ENUM_JOYPAD_START,  // event_joypad1_start,
    ENUM_JOYPAD_SELECT, // event_joypad1_select,
    ENUM_JOYPAD_UP,     // event_joypad1_up,
    ENUM_JOYPAD_DOWN,   // event_joypad1_down,
    ENUM_JOYPAD_LEFT,   // event_joypad1_left,
    ENUM_JOYPAD_RIGHT,  // event_joypad1_right,
};

char keyMap[] = {
    'U', // ENUM_JOYPAD_A
    'I', // ENUM_JOYPAD_B
    '1', // ENUM_JOYPAD_START
    'P', // ENUM_JOYPAD_SELECT
    'W', // ENUM_JOYPAD_UP
    'S', // ENUM_JOYPAD_DOWN
    'A', // ENUM_JOYPAD_LEFT
    'D', // ENUM_JOYPAD_RIGHT
};

char *keyMapHelp[] = {
    "U -> JOYPAD_A",      // ENUM_JOYPAD_A
    "I -> JOYPAD_B",      // ENUM_JOYPAD_B
    "1 -> JOYPAD_START",  // ENUM_JOYPAD_START
    "P -> JOYPAD_SELECT", // ENUM_JOYPAD_SELECT
    "W -> JOYPAD_UP",     // ENUM_JOYPAD_UP
    "S -> JOYPAD_DOWN",   // ENUM_JOYPAD_DOWN
    "A -> JOYPAD_LEFT",   // ENUM_JOYPAD_LEFT
    "D -> JOYPAD_RIGHT",  // ENUM_JOYPAD_RIGHT
};

/* ----------------------------------------------------------------------------
 * function declaration
 * ---------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * variable define
 * ---------------------------------------------------------------------------*/
uint8_t key = 0;
HANDLE hSerial = NULL;
/* ----------------------------------------------------------------------------
 * function define
 * ---------------------------------------------------------------------------*/
void KeyMap(char vkCode, WPARAM wParam) {
    if (vkCode == 0x1B) { // ESC key
        if (wParam == WM_KEYDOWN) {
            key = 0xFF; // Set all bits to 1 to indicate exit
            printf("ESC key pressed...\n");
        } else if (wParam == WM_KEYUP) {
            key = 0;
        }
    } else {
        for (int i = 0; i < 8; i++) {
            if (keyMap[i] == vkCode) {
                if (wParam == WM_KEYDOWN) {
                    key |= 1 << i;
                } else if (wParam == WM_KEYUP) {
                    key &= ~(1 << i);
                }
            }
        }
    }
}
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    DWORD bytesWritten;
    if (nCode >= 0) {

        KBDLLHOOKSTRUCT *pKeyInfo = (KBDLLHOOKSTRUCT *)lParam;
        switch (wParam) {
        case WM_KEYDOWN:
            // printf("WM_KEYDOWN: %c\n", pKeyInfo->vkCode);
            KeyMap(pKeyInfo->vkCode, wParam);
            WriteFile(hSerial, &key, 1, &bytesWritten, NULL);
            break;
        case WM_KEYUP:
            // printf("WM_KEYUP:%c\n", pKeyInfo->vkCode);
            KeyMap(pKeyInfo->vkCode, wParam);
            WriteFile(hSerial, &key, 1, &bytesWritten, NULL);
            break;
        default:
            printf("error wParam:0x%x\n", wParam);
            break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    char comPortName[10];
    int num;
    printf("Please input the COM port num (e.g., 6): ");

    scanf("%d", &num);
    if (num < 10) {
        sprintf(comPortName, "COM%d", num);
    } else {
        sprintf(comPortName, "\\\\.\\COM%d", num);
    }

    hSerial = CreateFile(comPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening port: %s, error:%d\n", comPortName, GetLastError());
        goto error;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hSerial, &dcb)) {
        printf("Error getting comm state: %d\n", GetLastError());
        CloseHandle(hSerial);
        goto error;
    }
    dcb.BaudRate = 1000000;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (SetCommState(hSerial, &dcb)) {
        printf("%s configured successfully. BaudRate: %d\n", comPortName, dcb.BaudRate);
    } else {
        printf("Error configuring %s error: %d\n", comPortName, GetLastError());
        CloseHandle(hSerial);
        goto error;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 500;
    SetCommTimeouts(hSerial, &timeouts);

    DWORD bytesWritten;
    WriteFile(hSerial, &key, 1, &bytesWritten, NULL);
    key = 0;

    for (size_t i = 0; i < 8; i++) {
        printf("%s\n", keyMapHelp[i]);
    }
    printf("Esc -> Reset System and Enter the next game\n");
    HHOOK hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
    MSG msg;
    while (1) {
        // printf("Waiting for key events...\n");
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0) > 0) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        } else {
            WaitMessage();
        }
    }
    return 0;
error:
    system("pause");
    return -1;
}