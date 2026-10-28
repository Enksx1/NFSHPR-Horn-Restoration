#include "Input.h"
#include "Config.h"
#include "Win32.h"

volatile i32 Input::padHeld;
volatile u32 Input::padPresses;

namespace {

struct XInputState {
    u32 packet;
    u16 buttons;
    u8 leftTrigger;
    u8 rightTrigger;
    short lx, ly, rx, ry;
};

struct JoyInfo {
    u32 size, flags, x, y, z, r, u, v, buttons, buttonNumber, pov, reserved1, reserved2;
};

using GetAsyncKeyStateFn = short(__stdcall*)(int);
using GetForegroundWindowFn = HANDLE(__stdcall*)();
using GetWindowThreadProcessIdFn = u32(__stdcall*)(HANDLE, u32*);
using XInputGetStateFn = u32(__stdcall*)(u32, XInputState*);
using JoyGetPosExFn = u32(__stdcall*)(u32, JoyInfo*);
using JoyGetDevCapsFn = u32(__stdcall*)(u64, void*, u32);
using JoyGetNumDevsFn = u32(__stdcall*)();

GetAsyncKeyStateFn getAsyncKeyState;
GetForegroundWindowFn getForegroundWindow;
GetWindowThreadProcessIdFn getWindowThreadProcessId;
XInputGetStateFn xinputGetState;
JoyGetPosExFn joyGetPosEx;
JoyGetDevCapsFn joyGetDevCaps;
JoyGetNumDevsFn joyGetNumDevs;

u32 padsConnected;
u32 nextPadProbe;
u32 probeSlot;
i32 sonyPad = -1;
u32 nextSonyScan;

template <typename T>
T Proc(HANDLE module, const char* name)
{
    return module ? reinterpret_cast<T>(GetProcAddress(module, name)) : nullptr;
}

}

void Input::Init()
{
    HANDLE user32 = LoadLibraryA("user32.dll");
    getAsyncKeyState = Proc<GetAsyncKeyStateFn>(user32, "GetAsyncKeyState");
    getForegroundWindow = Proc<GetForegroundWindowFn>(user32, "GetForegroundWindow");
    getWindowThreadProcessId = Proc<GetWindowThreadProcessIdFn>(user32, "GetWindowThreadProcessId");

    HANDLE xinput = LoadLibraryA("xinput1_4.dll");
    if (!xinput) xinput = LoadLibraryA("xinput1_3.dll");
    if (!xinput) xinput = LoadLibraryA("xinput9_1_0.dll");
    xinputGetState = Proc<XInputGetStateFn>(xinput, "XInputGetState");

    HANDLE winmm = LoadLibraryA("winmm.dll");
    joyGetPosEx = Proc<JoyGetPosExFn>(winmm, "joyGetPosEx");
    joyGetDevCaps = Proc<JoyGetDevCapsFn>(winmm, "joyGetDevCapsA");
    joyGetNumDevs = Proc<JoyGetNumDevsFn>(winmm, "joyGetNumDevs");
}

void Input::StartControllerThread()
{
    const Settings& s = Config::settings;
    if (!s.xboxMask && s.dsButton < 0) return;
    HANDLE t = CreateThread(nullptr, 0, ControllerThread, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
}

bool Input::GameFocused()
{
    if (!getForegroundWindow || !getWindowThreadProcessId) return true;
    HANDLE w = getForegroundWindow();
    if (!w) return false;
    u32 pid = 0;
    getWindowThreadProcessId(w, &pid);
    return pid == GetCurrentProcessId();
}

void Input::ReadKeyboard(bool& held, bool& tapped)
{
    held = tapped = false;
    u32 key = Config::settings.keyboardKey;
    if (!key || !getAsyncKeyState || !GameFocused()) return;
    short state = getAsyncKeyState(static_cast<int>(key));
    held = (state & 0x8000) != 0;
    tapped = !held && (state & 1);
}

bool Input::PollXbox(u32 now)
{
    u32 mask = Config::settings.xboxMask;
    if (!mask || !xinputGetState) return false;
    int probe = -1;
    if (static_cast<i32>(now - nextPadProbe) >= 0) {
        nextPadProbe = now + (padsConnected ? 5000 : 1250);
        for (u32 k = 0; k < 4; k++) {
            u32 slot = (probeSlot + k) & 3;
            if (!(padsConnected & (1u << slot))) {
                probe = static_cast<int>(slot);
                probeSlot = slot + 1;
                break;
            }
        }
    }
    bool held = false;
    for (u32 i = 0; i < 4; i++) {
        if (!(padsConnected & (1u << i)) && static_cast<int>(i) != probe) continue;
        XInputState st = {};
        if (xinputGetState(i, &st) != 0) {
            padsConnected &= ~(1u << i);
            continue;
        }
        padsConnected |= 1u << i;
        if (st.buttons & (mask & 0xFFFF)) held = true;
        if ((mask & Config::XboxLeftTrigger) && st.leftTrigger > 30) held = true;
        if ((mask & Config::XboxRightTrigger) && st.rightTrigger > 30) held = true;
    }
    return held;
}

bool Input::PollDs(u32 now)
{
    i32 button = Config::settings.dsButton;
    if (button < 0 || !joyGetPosEx || !joyGetDevCaps) return false;
    if (sonyPad < 0 && static_cast<i32>(now - nextSonyScan) >= 0) {
        nextSonyScan = now + 30000;
        u32 count = joyGetNumDevs ? joyGetNumDevs() : 16;
        if (count > 16) count = 16;
        for (u32 id = 0; id < count && sonyPad < 0; id++) {
            alignas(8) u8 caps[512] = {};
            if (joyGetDevCaps(id, caps, 404) != 0 || *reinterpret_cast<u16*>(caps) != 0x054C) continue;
            JoyInfo ji = {};
            ji.size = sizeof(ji);
            ji.flags = 0xFF;
            if (joyGetPosEx(id, &ji) == 0) sonyPad = static_cast<i32>(id);
        }
    }
    if (sonyPad < 0) return false;
    JoyInfo ji = {};
    ji.size = sizeof(ji);
    ji.flags = 0x80 | 0x40;
    if (joyGetPosEx(static_cast<u32>(sonyPad), &ji) != 0) {
        sonyPad = -1;
        nextSonyScan = now + 5000;
        return false;
    }
    if (button < 32) return (ji.buttons & (1u << button)) != 0;
    if (ji.pov == 0xFFFF) return false;
    u32 a = ji.pov;
    switch (button) {
    case Config::DsDpadUp: return a >= 31500 || a <= 4500;
    case Config::DsDpadRight: return a >= 4500 && a <= 13500;
    case Config::DsDpadDown: return a >= 13500 && a <= 22500;
    case Config::DsDpadLeft: return a >= 22500 && a <= 31500;
    default: return false;
    }
}

u32 __stdcall Input::ControllerThread(void*)
{
    nextPadProbe = nextSonyScan = GetTickCount();
    bool previous = false;
    for (;;) {
        u32 now = GetTickCount();
        bool held = false;
        if (GameFocused()) {
            held = PollXbox(now);
            if (PollDs(now)) held = true;
        }
        if (held && !previous) padPresses = padPresses + 1;
        previous = held;
        padHeld = held;
        Sleep(10);
    }
}
