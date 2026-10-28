#include "Hooks.h"
#include "Game.h"
#include "Shared.h"
#include "Win32.h"
#include <intrin.h>

extern "C" {
void hpr_frame_stub();
void hpr_send_stub();
void hpr_recv_stub();
void hpr_null1_stub();
void hpr_null2_stub();
void hpr_create_stub();
void hpr_unload_stub();
}

namespace {

struct Site {
    u32 offset;
    u8 length;
    u8 original[8];
    void (*stub)();
};

constexpr int SiteCount = 7;
constexpr int SoundPlayingSite = 3;

const Site sites[SiteCount] = {
    { Offsets::FrameHook, 5, { 0xE8, 0xB9, 0xA2, 0x01, 0x00 }, hpr_frame_stub },
    { Offsets::SendHook, 7, { 0x8B, 0xBC, 0x31, 0x84, 0x34, 0x00, 0x00 }, hpr_send_stub },
    { Offsets::ReceiveHook, 8, { 0x41, 0x89, 0x9C, 0x3C, 0x10, 0x4A, 0x00, 0x00 }, hpr_recv_stub },
    { Offsets::SoundPlayingGuard, 6, { 0xF3, 0x0F, 0x59, 0x74, 0x0A, 0x20 }, hpr_null1_stub },
    { Offsets::SoundStartGuard, 5, { 0xE8, 0xB0, 0xD1, 0x4C, 0x00 }, hpr_null2_stub },
    { Offsets::ResourceCreated, 7, { 0x4C, 0x8B, 0x85, 0xF0, 0x02, 0x00, 0x00 }, hpr_create_stub },
    { Offsets::ResourceUnload, 5, { 0x48, 0x89, 0x5C, 0x24, 0x08 }, hpr_unload_stub },
};

bool skip[SiteCount];
u8* trampolines;

bool Rel32(i64 v)
{
    return v >= -0x80000000LL && v <= 0x7FFFFFFFLL;
}

u8* SiteAddress(int i)
{
    return Game::exe + sites[i].offset;
}

bool PatchedByOtherMod(int i, const u8* bytes)
{
    return i == SoundPlayingSite && bytes[0] == 0xE9 && bytes[5] == 0x90;
}

bool Exchange16(u8* block, const u8* expected, const u8* desired)
{
    alignas(16) i64 compare[2];
    i64 value[2];
    memcpy(compare, expected, 16);
    memcpy(value, desired, 16);
    return _InterlockedCompareExchange128(reinterpret_cast<volatile i64*>(block), value[1], value[0], compare) != 0;
}

bool Exchange8(u8* block, const u8* expected, const u8* desired)
{
    i64 compare, value;
    memcpy(&compare, expected, 8);
    memcpy(&value, desired, 8);
    return _InterlockedCompareExchange64(reinterpret_cast<volatile i64*>(block), value, compare) == compare;
}

}

bool Hooks::SitesReady()
{
    HANDLE self = GetCurrentProcess();
    for (int i = 0; i < SiteCount; i++) {
        u8 bytes[8];
        u64 got = 0;
        if (!ReadProcessMemory(self, SiteAddress(i), bytes, sites[i].length, &got) || got != sites[i].length) return false;
        if (!memcmp(bytes, sites[i].original, sites[i].length)) skip[i] = false;
        else if (PatchedByOtherMod(i, bytes)) skip[i] = true;
        else return false;
    }
    return true;
}

u8* Hooks::AllocNear()
{
    u8* base = Game::exe;
    for (u64 d = 0x10000; d < 0x80000000ULL; d += 0x10000) {
        u64 a = (reinterpret_cast<u64>(base) - d) & ~0xFFFFULL;
        bool fits = true;
        for (int i = 0; i < SiteCount && fits; i++) {
            i64 from = reinterpret_cast<i64>(SiteAddress(i) + 5);
            if (!Rel32(static_cast<i64>(a + i * 16) - from) || !Rel32(static_cast<i64>(a + i * 16 + 15) - from)) fits = false;
        }
        if (!fits) break;
        void* p = VirtualAlloc(reinterpret_cast<void*>(a), 0x1000, Win::MemCommitReserve, Win::PageReadWrite);
        if (p) {
            if (reinterpret_cast<u64>(p) == a) return static_cast<u8*>(p);
            VirtualFree(p, 0, Win::MemRelease);
        }
    }
    return nullptr;
}

bool Hooks::Patch(u8* site, const u8* original, const u8* replacement, u32 length)
{
    u64 address = reinterpret_cast<u64>(site);
    u32 width = ((address & 7) + length <= 8) ? 8 : ((address & 15) + length <= 16) ? 16 : 0;
    if (!width) return false;
    u8* block = reinterpret_cast<u8*>(address & ~static_cast<u64>(width - 1));
    u32 offset = static_cast<u32>(site - block);

    alignas(16) u8 expected[16];
    alignas(16) u8 desired[16];
    memcpy(expected, block, width);
    if (memcmp(expected + offset, original, length)) return false;
    memcpy(desired, expected, width);
    memcpy(desired + offset, replacement, length);

    u32 old;
    if (!VirtualProtect(block, width, Win::PageExecuteReadWrite, &old)) return false;
    bool ok = width == 8 ? Exchange8(block, expected, desired) : Exchange16(block, expected, desired);
    u32 unused;
    VirtualProtect(block, width, old, &unused);
    FlushInstructionCache(GetCurrentProcess(), block, width);
    return ok;
}

Hooks::Result Hooks::Install()
{
    if (!trampolines) {
        trampolines = AllocNear();
        if (!trampolines) return Failed;
        for (int i = 0; i < SiteCount; i++) {
            u8* t = trampolines + i * 16;
            t[0] = 0xFF;
            t[1] = 0x25;
            *reinterpret_cast<u32*>(t + 2) = 0;
            *reinterpret_cast<u64*>(t + 6) = reinterpret_cast<u64>(sites[i].stub);
        }
        u32 old;
        if (!VirtualProtect(trampolines, 0x1000, Win::PageExecuteRead, &old) ||
            !FlushInstructionCache(GetCurrentProcess(), trampolines, 0x1000)) {
            VirtualFree(trampolines, 0, Win::MemRelease);
            trampolines = nullptr;
            return Failed;
        }
    }

    g_frame_orig = reinterpret_cast<u64>(Game::exe + Offsets::TrafficAudioUpdate);
    g_n2_call = reinterpret_cast<u64>(Game::exe + Offsets::SoundStartCall);
    g_n1_skip = reinterpret_cast<u64>(Game::exe + Offsets::SoundPlayingExit);
    g_n2_skip = reinterpret_cast<u64>(Game::exe + Offsets::SoundStartExit);

    u8 patch[SiteCount][8];
    for (int i = 0; i < SiteCount; i++) {
        i64 rel = reinterpret_cast<i64>(trampolines + i * 16) - reinterpret_cast<i64>(SiteAddress(i) + 5);
        if (!Rel32(rel)) return Failed;
        patch[i][0] = 0xE8;
        *reinterpret_cast<i32*>(patch[i] + 1) = static_cast<i32>(rel);
        for (int k = 5; k < sites[i].length; k++) patch[i][k] = 0x90;
    }

    for (int i = 0; i < SiteCount; i++) {
        if (skip[i] || Patch(SiteAddress(i), sites[i].original, patch[i], sites[i].length)) continue;
        for (int k = i - 1; k >= 0; k--) {
            if (!skip[k]) Patch(SiteAddress(k), patch[k], sites[k].original, sites[k].length);
        }
        return Changed;
    }
    return Installed;
}
