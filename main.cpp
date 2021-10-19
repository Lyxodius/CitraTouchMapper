#include <windows.h>
#include <XInput.h>
#include <vector>
#include <regex>
#include "rgb.cpp"

#define FRAME 16
#define DEADZONE 0.05f
#define MENU_BUTTON_COLOR COLORREF(0xf6e2c1)
#define BEAM_BUTTON_SELECTED_COLOR COLORREF(0xffae66)

bool getXInputState(XINPUT_STATE &state, bool &lastConnected) {
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
        auto *pThis = reinterpret_cast<MonitorRects *>(pData);
        pThis->rcMonitors.push_back(*lprcMonitor);
        return TRUE;
    }

    MonitorRects() {
        EnumDisplayMonitors(0, 0, callback, (LPARAM) this);
    }
};

COLORREF getPixelColor(Vector2 position) {
    HDC dc = GetDC(NULL);
    COLORREF color = GetPixel(dc, position.x * 2, position.y * 2);
    ReleaseDC(NULL, dc);
    return color;
}

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

HWND findWindow(const std::string &windowTitle) {
    std::vector<std::string> titles;
    EnumWindows(windowCallback, reinterpret_cast<LPARAM>(&titles));
    for (const auto &title: titles) {
        std::regex self_regex(windowTitle);
        if (std::regex_search(title, self_regex)) {
            return FindWindow(nullptr, title.c_str());
        }
    }
    return nullptr;
}

bool isBrighterOrEqual(COLORREF color1, COLORREF color2) {
    int sum1 = GetRValue(color1) + GetGValue(color1) + GetBValue(color1);
    int sum2 = GetRValue(color2) + GetGValue(color2) + GetBValue(color2);
    return sum1 >= sum2;
}

void printMouseData() {
    POINT point;
    GetCursorPos(&point);
    COLORREF color = getPixelColor(Vector2(point.x, point.y));
    hsv hsvColor = rgb2hsv(rgb(color));
    bool brighterThan = isBrighterOrEqual(color, BEAM_BUTTON_SELECTED_COLOR);
    printf("x: %ld, y: %ld, color: %.6lx, brighterThan: %d, h: %f, s: %f, v: %f\n", point.x, point.y, color,
           brighterThan, hsvColor.h, hsvColor.s, hsvColor.v);
}

int main() {
    MonitorRects monitors;
    if (monitors.rcMonitors.size() < 2) {
        printf("There is no second monitor.\n");
        exit(1);
    }

    tagRECT secondMonitor = monitors.rcMonitors[1];
    int x = 3332;

    Vector2 menuButtonPosition = Vector2(2266, 1022);
    Vector2 positions[3] = {Vector2(x, 92), Vector2(x, 430), Vector2(x, 767)};

    bool lastConnected = false;

    HWND citraGame = findWindow(R"(Citra\s\w+\s\|\sHEAD)");
    if (citraGame) SetForegroundWindow(citraGame);

    while (true) {
        XINPUT_STATE state;
        bool r1Pressed = false;
        bool lTriggerPressed = false;
        bool rTriggerPressed = false;
        bool l3Pressed = false;
        bool r3Pressed = false;
        int selectedBeamButton = 0;
        int defaultBeam = 0;
        int defaultMissile = 0;
        while (getXInputState(state, lastConnected) == ERROR_SUCCESS) {
            printMouseData();

            COLORREF menuButtonPositionColor = getPixelColor(menuButtonPosition);

            if (menuButtonPositionColor == MENU_BUTTON_COLOR) {
                bool r1 = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
                if (r1 && !r1Pressed) {
                    r1Pressed = true;
                } else if (!r1) {
                    r1Pressed = false;
                }

                bool l3 = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
                if (l3 && !l3Pressed) {
                    l3Pressed = true;
                    if (!r1Pressed) {
                        defaultBeam = !defaultBeam;
                    } else {
                        defaultMissile = !defaultMissile;
                    }
                } else if (!l3) {
                    l3Pressed = false;
                }

                float leftTrigger = (float) state.Gamepad.bLeftTrigger / 255;
                if (leftTrigger >= DEADZONE && !lTriggerPressed) {
                    lTriggerPressed = true;
                } else if (leftTrigger < DEADZONE && lTriggerPressed) {
                    lTriggerPressed = false;
                }

                selectedBeamButton = !r1Pressed ? defaultBeam : defaultMissile;
                float rightTrigger = (float) state.Gamepad.bRightTrigger / 255;
                if (rightTrigger >= DEADZONE && !r1Pressed) {
                    selectedBeamButton = 2;
                }

                bool morphBall = false;
                for (int i = 0; i < 2; i++) {
                    COLORREF color = getPixelColor(positions[i]);
                    hsv h = rgb2hsv(rgb(color));
                    if (h.v < 0.4) {
                        morphBall = true;
                        break;
                    }
                }

                hsv selectedBeamButtonColor = rgb2hsv(rgb(getPixelColor(positions[selectedBeamButton])));
                if (selectedBeamButtonColor.v < 1 && !morphBall || selectedBeamButtonColor.v < 0.51 && morphBall) {
                    clickAt(positions[selectedBeamButton]);
                    SetCursorPos(secondMonitor.right, secondMonitor.bottom);
                }
            }

            bool r3 = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
            if (r3 && !r3Pressed) {
                r3Pressed = true;

                HWND citra = findWindow(R"(Citra\s\w+\s\d+)");
                citraGame = findWindow(R"(Citra\s\w+\s\|\sHEAD)");

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
}