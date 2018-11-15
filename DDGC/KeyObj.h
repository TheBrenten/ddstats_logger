#pragma once

class KeyObj {
    int code;
    bool state = false;
public:
    KeyObj(int code) {
        this->code = code;
    }
    bool justPressed() {
        bool oldState = state;
        state = GetAsyncKeyState(code);
        if (!oldState && state) {
            return true;
        }
        return false;
    }
    bool isPressed() {
        return GetAsyncKeyState(code);
    }
};

KeyObj kF1 (VK_F1 );
KeyObj kF10(VK_F10);
KeyObj kF11(VK_F11);
KeyObj kF12(VK_F12);
