#include <windows.h>
#include <iostream>
#include <XInput.h>

bool getState(XINPUT_STATE &state) {
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    bool connected = XInputGetState(0, &state);
    if (connected != ERROR_SUCCESS) {
        printf("Controller is not connected.");
    }
    return connected;
}

int main() {
    XINPUT_STATE state;
    while (getState(state) == ERROR_SUCCESS) {
        float leftTrigger = (float) state.Gamepad.bLeftTrigger / 255;
        printf("Trigger: %f\n", leftTrigger);
        Sleep(16);
        getState(state);
    }

    return 0;
}