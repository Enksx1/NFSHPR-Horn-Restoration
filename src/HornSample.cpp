#include "HornSample.h"
#include "Config.h"
#include "Game.h"
#include "HornPlayer.h"
#include "Str.h"
#include "Win32.h"
#include <intrin.h>

u8* volatile HornSample::live;
volatile u32 HornSample::gameThread;
volatile i32 HornSample::busy;
volatile i32 HornSample::stopRequest;
volatile i32 HornSample::voicesActive;

namespace {

const u8 Signature[12] = { 0x48, 0x00, 0x00, 0x0C, 0x15, 0x04, 0xBB, 0x80, 0x40, 0x00, 0x89, 0xF9 };

u8* volatile candidates[32];
volatile i32 candidateCount;
volatile long lockFlag;

u8* checkedEntry;
u8* checkedWave;

u8* privateWave;
bool privateFailed;
u8* customWave;
u32 customSize;
bool customLooping;

u8* clonePage;
u8* cloneSource;
u8* cloneWave;
u8* cloneCurrent;
u32 cloneNext;

u32 BigEndian32(const u8* p)
{
    return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) | (static_cast<u32>(p[2]) << 8) | p[3];
}
}

void HornSample::Lock()
{
    while (_InterlockedExchange(&lockFlag, 1)) _mm_pause();
}

void HornSample::Unlock()
{
    _InterlockedExchange(&lockFlag, 0);
}

void HornSample::LoadCustom(const char* dir)
{
    const Settings& cfg = Config::settings;
    if (!cfg.customSound || !cfg.customFile[0]) return;
    char path[600];
    path[0] = 0;
    bool absolute = cfg.customFile[1] == ':' || cfg.customFile[0] == '\\' || cfg.customFile[0] == '/';
    if (!absolute) Str::Append(path, dir);
    Str::Append(path, cfg.customFile);

    HANDLE f = CreateFileA(path, Win::GenericRead, 1, nullptr, 3, 0x80, nullptr);
    if (!Win::Valid(f)) return;
    const u32 maxSize = 4u << 20;
    u8* buffer = static_cast<u8*>(VirtualAlloc(nullptr, 0x80 + maxSize + 0x100, Win::MemCommitReserve, Win::PageReadWrite));
    u32 read = 0;
    if (buffer) ReadFile(f, buffer + 0x80, maxSize, &read, nullptr);
    CloseHandle(f);
    if (!buffer) return;

    u8* sps = buffer + 0x80;
    if (sps[0] != 0x48 && read >= 0x100 && sps[0x80] == 0x48) {
        read -= 0x80;
        memcpy(sps, sps + 0x80, read);
    }
    if (read < 0x20 || sps[0] != 0x48) {
        VirtualFree(buffer, 0, Win::MemRelease);
        return;
    }
    u32 headerSize = BigEndian32(sps) & 0xFFFFFF;
    u32 h1 = BigEndian32(sps + 4);
    u32 h2 = BigEndian32(sps + 8);
    u32 channels = ((h1 >> 18) & 0x3F) + 1;
    u32 rate = h1 & 0x3FFFF;
    u32 samples = h2 & 0x1FFFFFFF;

    u32 offset = headerSize, end = 0;
    while (offset + 4 <= read) {
        u32 type = sps[offset] & 0x7F;
        u32 size = BigEndian32(sps + offset) & 0xFFFFFF;
        if (!size || (type != 0x44 && type != 0x45)) break;
        offset += size;
        if (type == 0x45) {
            end = offset;
            break;
        }
    }
    if (!end || end > read || !rate) {
        VirtualFree(buffer, 0, Win::MemRelease);
        return;
    }

    memset(buffer, 0, 0x80);
    *reinterpret_cast<u8**>(buffer) = sps;
    *reinterpret_cast<float*>(buffer + 0x20) = 1.0f;
    *reinterpret_cast<float*>(buffer + 0x24) = static_cast<float>(samples) * 1000.0f / static_cast<float>(rate);
    *reinterpret_cast<u32*>(buffer + 0x28) = end;
    *reinterpret_cast<u32*>(buffer + 0x34) = channels;

    customWave = buffer;
    customSize = 0x80 + end;
    customLooping = ((h2 >> 29) & 1) != 0;
}

void HornSample::FindLoaded()
{
    const u64 chunk = 1 << 20;
    u8* buffer = static_cast<u8*>(VirtualAlloc(nullptr, chunk, Win::MemCommitReserve, Win::PageReadWrite));
    if (!buffer) return;
    HANDLE self = GetCurrentProcess();
    u8* address = reinterpret_cast<u8*>(0x10000);
    MemoryInfo m;
    while (VirtualQuery(address, &m, sizeof(m))) {
        u8* base = static_cast<u8*>(m.BaseAddress);
        u8* next = base + m.RegionSize;
        if (m.State == Win::MemCommit && m.Type == 0x20000 && m.Protect == Win::PageReadWrite) {
            for (u8* p = base; p < next; p += chunk) {
                u64 size = static_cast<u64>(next - p) < chunk ? static_cast<u64>(next - p) : chunk;
                u64 got = 0;
                if (!ReadProcessMemory(self, p, buffer, size, &got)) continue;
                for (u64 o = 0; o + 0x30 <= got; o += 8) {
                    if (*reinterpret_cast<u32*>(buffer + o) != HornId) continue;
                    u8* wave = *reinterpret_cast<u8**>(buffer + o + 0x28);
                    u8 header[0x90];
                    u64 n = 0;
                    if (!wave || !ReadProcessMemory(self, wave, header, sizeof(header), &n) || n != sizeof(header)) continue;
                    if (*reinterpret_cast<u8**>(header) != wave + 0x80 || memcmp(header + 0x80, Signature, 12)) continue;
                    OnCreated(p + o);
                }
            }
        }
        if (next <= address) break;
        address = next;
    }
    VirtualFree(buffer, 0, Win::MemRelease);
}

bool HornSample::Valid(u8* entry)
{
    if (!entry) return false;
    if (entry != checkedEntry) {
        if (!Game::Readable(entry, 0x30) || *reinterpret_cast<u32*>(entry) != HornId) return false;
        u8* wave = *reinterpret_cast<u8**>(entry + 0x28);
        if (!wave || !Game::Readable(wave, 0x90)) return false;
        checkedEntry = entry;
        checkedWave = wave;
    }
    u8* wave = *reinterpret_cast<u8**>(entry + 0x28);
    if (*reinterpret_cast<u32*>(entry) != HornId || wave != checkedWave ||
        *reinterpret_cast<u8**>(wave) != wave + 0x80 || memcmp(wave + 0x80, Signature, 12)) {
        checkedEntry = nullptr;
        return false;
    }
    return true;
}

void HornSample::OnCreated(u8* entry)
{
    if (!entry || *reinterpret_cast<u32*>(entry) != HornId) return;
    Lock();
    i32 n = candidateCount;
    i32 i = 0;
    while (i < n && candidates[i] != entry) i++;
    if (i == n) {
        if (n == 32) {
            for (i32 k = 1; k < 32; k++) candidates[k - 1] = candidates[k];
            n = 31;
        }
        candidates[n++] = entry;
        candidateCount = n;
    }
    Unlock();
}

void HornSample::OnUnload(u8* entry)
{
    if (!entry || *reinterpret_cast<u32*>(entry) != HornId) return;
    Lock();
    i32 n = candidateCount, j = 0;
    for (i32 i = 0; i < n; i++) {
        if (candidates[i] != entry) candidates[j++] = candidates[i];
    }
    candidateCount = j;
    bool wasLive = live == entry;
    if (wasLive) live = nullptr;
    Unlock();
    if (!wasLive) return;
    if (GetCurrentThreadId() == gameThread) {
        HornPlayer::StopAll();
        return;
    }
    stopRequest = 1;
    u32 start = GetTickCount();
    while ((busy || stopRequest || voicesActive) && GetTickCount() - start < 250) Sleep(1);
}

u8* HornSample::PickLocked()
{
    for (i32 i = candidateCount - 1; i >= 0; i--) {
        if (Valid(candidates[i])) return candidates[i];
    }
    return nullptr;
}

u8* HornSample::Select(u32 frame)
{
    Lock();
    u8* e = live;
    if (e && !Valid(e)) {
        live = nullptr;
        e = nullptr;
    }
    if (!e && (frame & 7) == 0 && (e = PickLocked())) live = e;
    Unlock();
    return e;
}

u8* HornSample::BuildPrivate(u8* entry)
{
    u8* wave = *reinterpret_cast<u8**>(entry + 0x28);
    u32 size = *reinterpret_cast<u32*>(entry + 8);
    if (size < 0x100 || size > (1u << 20)) return nullptr;
    u8* copy = static_cast<u8*>(VirtualAlloc(nullptr, size + 0x100, Win::MemCommitReserve, Win::PageReadWrite));
    if (!copy) return nullptr;
    memcpy(copy, wave, size);
    *reinterpret_cast<u8**>(copy) = copy + 0x80;
    if (memcmp(copy + 0x80, Signature, 12)) return nullptr;
    return copy;
}

u8* HornSample::CloneFor(u8* entry, u8* wave)
{
    if (entry == cloneSource && wave == cloneWave) return cloneCurrent;
    if (!clonePage) {
        clonePage = static_cast<u8*>(VirtualAlloc(nullptr, 0x1000, Win::MemCommitReserve, Win::PageReadWrite));
        if (!clonePage) return nullptr;
    }
    u8* clone = clonePage + (cloneNext++ % 42) * 0x60;
    memcpy(clone, entry, 0x60);
    *reinterpret_cast<u8**>(clone + 0x28) = wave;
    if (wave == customWave) *reinterpret_cast<u32*>(clone + 8) = customSize;
    cloneSource = entry;
    cloneWave = wave;
    cloneCurrent = clone;
    return clone;
}

u8* HornSample::PlaybackEntry(u8* entry)
{
    u8* wave = customWave;
    if (!wave && Config::settings.privateCopy && !privateFailed) {
        if (!privateWave && !(privateWave = BuildPrivate(entry))) privateFailed = true;
        wave = privateWave;
    }
    if (!wave) return entry;
    u8* clone = CloneFor(entry, wave);
    return clone ? clone : entry;
}

bool HornSample::Looping()
{
    return customWave && customLooping;
}
