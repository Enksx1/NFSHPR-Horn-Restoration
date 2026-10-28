#pragma once
#include "Types.h"

class Hooks {
public:
    enum Result { Failed, Installed, Changed };

    static bool SitesReady();
    static Result Install();

private:
    static u8* AllocNear();
    static bool Patch(u8* site, const u8* original, const u8* replacement, u32 length);
};
