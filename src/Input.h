#pragma once
#include "Types.h"

class Input {
public:
    static void Init();
    static void StartControllerThread();
    static bool GameFocused();
    static void ReadKeyboard(bool& held, bool& tapped);

    static volatile i32 padHeld;
    static volatile u32 padPresses;

private:
    static u32 __stdcall ControllerThread(void*);
    static bool PollXbox(u32 now);
    static bool PollDs(u32 now);
};
