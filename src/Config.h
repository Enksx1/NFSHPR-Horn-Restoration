#pragma once
#include "Types.h"

struct Settings {
    u32 keyboardKey;
    u32 xboxMask;
    i32 dsButton;
    float playerGain;
    float competitorGain;
    bool privateCopy;
    bool customSound;
    char customFile[260];
};

class Config {
public:
    static constexpr u32 XboxLeftTrigger = 0x10000;
    static constexpr u32 XboxRightTrigger = 0x20000;
    static constexpr i32 DsDpadUp = 100;
    static constexpr i32 DsDpadRight = 101;
    static constexpr i32 DsDpadDown = 102;
    static constexpr i32 DsDpadLeft = 103;

    static Settings settings;
    static void Load(const char* dir);

private:
    static bool ParseKey(const char* raw, u32& out);
    static bool ParseXbox(const char* raw, u32& out);
    static bool ParseDs(const char* raw, i32& out);
    static bool ParseSwitch(const char* raw, bool& out);
    static void Apply(const char* key, const char* value);
};
