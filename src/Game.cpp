#include "Game.h"
#include "Win32.h"

u8* Game::exe;

namespace {

using PoolAllocFn = u8* (*)(u8*);
using FadeFn = char (*)(u8*, u32, float);
using LocalIndexFn = i32 (*)(u8*);
using VehicleFn = u8* (*)(u8*, i32);
using SoundInitFn = void (*)(u8*, u32);
using SoundStartFn = void* (*)(u8*, void*, void*);
using SoundStopFn = void (*)(u8*);
using SoundVecFn = void (*)(u8*, void*);

template <typename T>
T At(u32 offset)
{
    return reinterpret_cast<T>(Game::exe + offset);
}

template <typename T>
T Virtual(u8* object, u32 slot)
{
    return reinterpret_cast<T>((*reinterpret_cast<void***>(object))[slot]);
}

u8* Deref(u32 offset)
{
    return *reinterpret_cast<u8**>(Game::exe + offset);
}

}

void Game::Init()
{
    exe = static_cast<u8*>(GetModuleHandleA(nullptr));
}

bool Game::Readable(const void* p, u64 n)
{
    MemoryInfo m;
    if (!VirtualQuery(p, &m, sizeof(m))) return false;
    if (m.State != Win::MemCommit || (m.Protect & 0x101) || !m.Protect) return false;
    return static_cast<const u8*>(p) + n <= static_cast<u8*>(m.BaseAddress) + m.RegionSize;
}

u8* Game::LocalVehicle()
{
    u8* players = Deref(Offsets::PlayerManager);
    if (!players) return nullptr;
    i32 index = At<LocalIndexFn>(Offsets::LocalVehicleIndex)(players + 0x15E1D8);
    if (index == -1) return nullptr;
    u8* vehicles = Deref(Offsets::VehicleManager);
    if (!vehicles) return nullptr;
    return At<VehicleFn>(Offsets::VehicleByIndex)(vehicles, index);
}

u8* Game::VehicleAt(i32 index)
{
    u8* vehicles = Deref(Offsets::VehicleManager);
    if (!vehicles || !*reinterpret_cast<u8**>(vehicles + 8)) return nullptr;
    return At<VehicleFn>(Offsets::VehicleByIndex)(vehicles, index);
}

u8* Game::AudioManager()
{
    u8* audio = Deref(Offsets::AudioSystem);
    return audio ? *reinterpret_cast<u8**>(audio + 8) : nullptr;
}

u8* Game::AllocSound(u8* pool)
{
    return At<PoolAllocFn>(Offsets::PoolAlloc)(pool);
}

void Game::FadeIn(u8* pool, u32 id, float seconds)
{
    At<FadeFn>(Offsets::FadeIn)(pool, id & 0xFFFFFF, seconds);
}

void Game::FadeOut(u8* pool, u32 id, float seconds)
{
    At<FadeFn>(Offsets::FadeOut)(pool, id & 0xFFFFFF, seconds);
}

void Game::SoundInit(u8* sound, u32 id)
{
    Virtual<SoundInitFn>(sound, 0)(sound, id);
}

void Game::SoundStart(u8* sound, void* wave, void* channel)
{
    Virtual<SoundStartFn>(sound, 3)(sound, wave, channel);
}

void Game::SoundStop(u8* sound)
{
    Virtual<SoundStopFn>(sound, 5)(sound);
}

void Game::SoundAttach(u8* sound, u8* vehicle)
{
    Virtual<SoundVecFn>(sound, 11)(sound, vehicle + 0xA0);
    Virtual<SoundVecFn>(sound, 13)(sound, vehicle + 0xE0);
}
