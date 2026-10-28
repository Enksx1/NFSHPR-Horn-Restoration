#pragma once
#include "Types.h"

class HornSample {
public:
    static constexpr u32 HornId = 0x1B5B2;

    static void LoadCustom(const char* dir);
    static void FindLoaded();
    static void OnCreated(u8* entry);
    static void OnUnload(u8* entry);

    static u8* Select(u32 frame);
    static bool Valid(u8* entry);
    static u8* PlaybackEntry(u8* entry);
    static bool Looping();

    static u8* volatile live;
    static volatile u32 gameThread;
    static volatile i32 busy;
    static volatile i32 stopRequest;
    static volatile i32 voicesActive;

private:
    static void Lock();
    static void Unlock();
    static u8* PickLocked();
    static u8* BuildPrivate(u8* entry);
    static u8* CloneFor(u8* entry, u8* wave);
};
