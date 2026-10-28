#pragma once
#include "Types.h"

struct Voice {
    u8* sound;
    u32 id;
    float volume;
    bool wasActive;
    bool muted;
    bool autoReleased;
    bool looping;
    u32 startTick;
    u32 releaseTick;
    u32 playTick;
    i32 autoReleaseAt;
    u32 vehicleTag;
};

class HornPlayer {
public:
    static void Frame(u8* trafficAudio);
    static void StopAll();

private:
    static bool Alive(const Voice& v);
    static void Stop(u8* sound, u32 id);
    static bool Draining();
    static void Release(Voice& v);
    static void FadeOut(Voice& v, u32 now);
    static u8* Play(u8* vehicle, u32 channel, float volume, u32& id, float& duration);
    static bool Start(Voice& v, u8* vehicle, u32 channel, u32 now);
    static void Drive(Voice& v, bool active, u8* vehicle, u32 channel, u32 now);
    static void UpdateLocal(u8* vehicle, u32 now);
    static void UpdateRemote(u32 now);
};
