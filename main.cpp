#include <windows.h>
#include <XInput.h>
#include <vector>
#include <regex>

#define FRAME 16
#define DEADZONE 0.5f

bool getState(XINPUT_STATE &state, bool &lastConnected) {
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    bool connected = XInputGetState(0, &state);
    if (connected == ERROR_SUCCESS) {
        if (!lastConnected) {
            printf("Controller is connected!\nYou can now minimize this window and start playing.\n");
        }
        lastConnected = true;
    } else {
        lastConnected = false;
        printf("Controller is not connected.\nPress any key to try again.\n");
    }
    return connected;
}

struct Vector2 {
    int x;
    int y;

    Vector2(int x, int y) {
        this->x = x;
        this->y = y;
    }
};

void clickAt(Vector2 position) {
    SetCursorPos(position.x, position.y);

    INPUT input = {0};

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    Sleep(FRAME * 2);

    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(1, &input, sizeof(INPUT));
}

struct MonitorRects {
    std::vector<RECT> rcMonitors;

    static BOOL CALLBACK callback(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData) {
        MonitorRects *pThis = reinterpret_cast<MonitorRects *>(pData);
        pThis->rcMonitors.push_back(*lprcMonitor);
        return TRUE;
    }

    MonitorRects() {
        EnumDisplayMonitors(0, 0, callback, (LPARAM) this);
    }
};

//COLORREF getPixelColor(Vector2 position) {
//    HDC dc = GetDC(NULL);
//    COLORREF color = GetPixel(dc, position.x, position.y);
//    ReleaseDC(NULL, dc);
//    return color;
//}

BOOL CALLBACK windowCallback(HWND hWnd, LPARAM lParam) {
    const DWORD TITLE_SIZE = 1024;
    CHAR windowTitle[TITLE_SIZE];

    GetWindowText(hWnd, windowTitle, TITLE_SIZE);

    int length = GetWindowTextLength(hWnd);
    std::string title(&windowTitle[0]);
    if (!IsWindowVisible(hWnd) || length == 0 || title == "Program Manager") {
        return TRUE;
    }

    std::vector<std::string> &titles = *reinterpret_cast<std::vector<std::string> *>(lParam);
    titles.push_back(title);

    return TRUE;
}

HWND findWindow(std::string windowTitle) {
    std::vector<std::string> titles;
    EnumWindows(windowCallback, reinterpret_cast<LPARAM>(&titles));
    for (const auto &title: titles) {
        std::regex self_regex(windowTitle);
        if (std::regex_search(title, self_regex)) {
            return FindWindow(NULL, title.c_str());
        }
    }
}

int main() {
    MonitorRects monitors;
    if (monitors.rcMonitors.size() < 2) {
        printf("There is no second monitor.\n");
        exit(1);
    }

    tagRECT secondMonitor = monitors.rcMonitors[1];
    int x = secondMonitor.right - 397;

    Vector2 positions[3] = {Vector2(x, 202), Vector2(x, 540), Vector2(x, 877)};
    int positionIndex = 0;

    bool lastConnected = false;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        XINPUT_STATE state;
        bool lTriggerPressed = false;
        bool rTriggerPressed = false;
        bool r3Pressed = false;
        while (getState(state, lastConnected) == ERROR_SUCCESS) {
            float leftTrigger = (float) state.Gamepad.bLeftTrigger / 255;
            if (leftTrigger >= DEADZONE && !lTriggerPressed) {
                lTriggerPressed = true;
                positionIndex++;
                if (positionIndex >= 2) positionIndex = 0;
                clickAt(positions[positionIndex]);
                SetCursorPos(secondMonitor.right, secondMonitor.bottom);
            } else if (leftTrigger < DEADZONE) {
                lTriggerPressed = false;
            }

            float rightTrigger = (float) state.Gamepad.bRightTrigger / 255;
            if (rightTrigger >= DEADZONE && !rTriggerPressed) {
                rTriggerPressed = true;
                clickAt(positions[2]);
                SetCursorPos(secondMonitor.right, secondMonitor.bottom);
            } else if (rightTrigger < DEADZONE && rTriggerPressed) {
                rTriggerPressed = false;
                clickAt(positions[positionIndex]);
                SetCursorPos(secondMonitor.right, secondMonitor.bottom);
            }

            bool r3 = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
            if (r3 && !r3Pressed) {
                r3Pressed = true;

                HWND citra = findWindow(R"(Citra\s\w+\s\d+)");
                HWND citraGame = findWindow(R"(Citra\s\w+\s\|\sHEAD)");

                SetForegroundWindow(citra);
                SendMessage(citra, WM_KEYDOWN, VK_F4, 0);
                SendMessage(citra, WM_KEYUP, VK_F4, 0);
                SetForegroundWindow(citraGame);
            } else if (!r3) {
                r3Pressed = false;
            }

            Sleep(FRAME);
        }
        system("pause");
    }
#pragma clang diagnostic pop
}

//COLORREF color = getPixelColor(Vector2(positions[2].x * 2, positions[2].y * 2));