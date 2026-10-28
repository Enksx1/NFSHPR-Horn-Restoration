#pragma once
#include "Types.h"

namespace Offsets {
constexpr u32 FrameHook = 0x18BAF2;
constexpr u32 TrafficAudioUpdate = 0x1A5DB0;
constexpr u32 SendHook = 0x1EFBD1;
constexpr u32 ReceiveHook = 0x1F0213;
constexpr u32 SoundPlayingGuard = 0x5D1256;
constexpr u32 SoundPlayingExit = 0x5D1327;
constexpr u32 SoundStartGuard = 0x5D115B;
constexpr u32 SoundStartCall = 0xA9E310;
constexpr u32 SoundStartExit = 0x5D132F;
constexpr u32 ResourceCreated = 0x5E5F60;
constexpr u32 ResourceUnload = 0x5E74C0;
constexpr u32 PoolAlloc = 0x5D4430;
constexpr u32 FadeIn = 0x5D3F10;
constexpr u32 FadeOut = 0x5D3B20;
constexpr u32 LocalVehicleIndex = 0x30D8A0;
constexpr u32 VehicleByIndex = 0x1F4FB0;
constexpr u32 PlayerManager = 0x1279218;
constexpr u32 VehicleManager = 0x129C4C8;
constexpr u32 AudioSystem = 0x129C510;
constexpr u32 TrafficSoundPool = 0xC7190;
}

namespace Channels {
constexpr u32 PlayerHorn = 0x17490;
constexpr u32 CompetitorHorn = 0x17CF0;
}

class Game {
public:
    static u8* exe;

    static void Init();
    static bool Readable(const void* p, u64 n);
    static u8* LocalVehicle();
    static u8* VehicleAt(i32 index);
    static u8* AudioManager();

    static u8* AllocSound(u8* pool);
    static void FadeIn(u8* pool, u32 id, float seconds);
    static void FadeOut(u8* pool, u32 id, float seconds);

    static void SoundInit(u8* sound, u32 id);
    static void SoundStart(u8* sound, void* wave, void* channel);
    static void SoundStop(u8* sound);
    static void SoundAttach(u8* sound, u8* vehicle);
    static u32 SoundId(const u8* sound) { return *reinterpret_cast<const u32*>(sound + 0xB4); }
    static u32 SoundState(const u8* sound) { return *reinterpret_cast<const u32*>(sound + 0xAC); }
    static void SoundVolume(u8* sound, float v) { *reinterpret_cast<float*>(sound + 0x90) = v; }

    static constexpr u32 SoundIdle = 0xB;
};
