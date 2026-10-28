#include "HornPlayer.h"
#include "Config.h"
#include "Game.h"
#include "HornSample.h"
#include "Input.h"
#include "Shared.h"
#include "Win32.h"

namespace {

constexpr u32 MinHornMs = 50;
constexpr float AttackFade = 0.05f;
constexpr float ReleaseFade = 0.1f;
constexpr u32 ReleaseStopMs = 120;
constexpr u32 MaxRemoteMs = 15000;
constexpr int RemoteSlots = 32;

struct StoppedSound {
    u8* sound;
    u32 id;
};

Voice local;
Voice remote[RemoteSlots];
StoppedSound draining[64];
int drainingCount;

u8* pool;
u8* lastVehicle;
u8* lastEntry;
u32 frameCount;
u32 minUntil;
bool minArmed;
bool previousHeld;
u32 pressesSeen;

}

bool HornPlayer::Alive(const Voice& v)
{
    return v.sound && Game::SoundId(v.sound) == v.id && Game::SoundState(v.sound) != Game::SoundIdle;
}

void HornPlayer::Stop(u8* sound, u32 id)
{
    Game::SoundStop(sound);
    if (drainingCount < 64) draining[drainingCount++] = { sound, id };
}

bool HornPlayer::Draining()
{
    int j = 0;
    for (int i = 0; i < drainingCount; i++) {
        u8* s = draining[i].sound;
        if (Game::SoundId(s) == draining[i].id && Game::SoundState(s) != Game::SoundIdle) draining[j++] = draining[i];
    }
    drainingCount = j;
    return j != 0;
}

void HornPlayer::Release(Voice& v)
{
    if (Alive(v)) Stop(v.sound, v.id);
    v.sound = nullptr;
    v.wasActive = false;
}

void HornPlayer::StopAll()
{
    Release(local);
    for (Voice& v : remote) Release(v);
}

void HornPlayer::FadeOut(Voice& v, u32 now)
{
    Game::FadeOut(pool, v.id, ReleaseFade);
    v.releaseTick = now;
}

u8* HornPlayer::Play(u8* vehicle, u32 channel, float volume, u32& id, float& duration)
{
    u8* entry = HornSample::live;
    if (!entry || !HornSample::Valid(entry)) return nullptr;
    u8* playback = HornSample::PlaybackEntry(entry);
    u8* audio = Game::AudioManager();
    if (!pool || !audio) return nullptr;
    u8* sound = Game::AllocSound(pool);
    if (!sound) return nullptr;
    u32& counter = *reinterpret_cast<u32*>(pool + 0x2C);
    counter++;
    Game::SoundInit(sound, counter & 0xFFFFFF);
    if (vehicle) Game::SoundAttach(sound, vehicle);
    struct {
        u8* entry;
        u32 offset;
        u32 one;
    } wave = { playback, 0, 1 };
    alignas(16) u8 mixer[16];
    memcpy(mixer, audio + channel, 16);
    Game::SoundStart(sound, &wave, mixer);
    Game::SoundVolume(sound, volume);
    Game::FadeIn(pool, Game::SoundId(sound), AttackFade);
    id = Game::SoundId(sound);
    duration = *reinterpret_cast<float*>(*reinterpret_cast<u8**>(playback + 0x28) + 0x24);
    return sound;
}

bool HornPlayer::Start(Voice& v, u8* vehicle, u32 channel, u32 now)
{
    u32 id = 0;
    float duration = 0.0f;
    v.sound = Play(vehicle, channel, v.volume, id, duration);
    v.id = id;
    if (!v.sound) return false;
    v.playTick = now;
    v.autoReleased = false;
    v.looping = HornSample::Looping();
    v.autoReleaseAt = static_cast<i32>(duration - ReleaseFade * 1000.0f);
    return true;
}

void HornPlayer::Drive(Voice& v, bool active, u8* vehicle, u32 channel, u32 now)
{
    bool alive = Alive(v);
    if (active) {
        if (!vehicle) {
            v.wasActive = false;
            return;
        }
        if (!v.wasActive) {
            if (alive) Stop(v.sound, v.id);
            v.wasActive = Start(v, vehicle, channel, now);
            return;
        }
        if (!alive) {
            v.sound = nullptr;
            if (v.looping) Start(v, vehicle, channel, now);
        } else {
            Game::SoundVolume(v.sound, v.volume);
            if (!v.looping && !v.autoReleased && static_cast<i32>(now - v.playTick) >= v.autoReleaseAt) {
                FadeOut(v, now);
                v.autoReleased = true;
            }
            Game::SoundAttach(v.sound, vehicle);
        }
    } else if (!alive) {
        v.sound = nullptr;
    } else if (v.wasActive && !v.autoReleased) {
        FadeOut(v, now);
        if (vehicle) Game::SoundAttach(v.sound, vehicle);
    } else if (now - v.releaseTick >= ReleaseStopMs) {
        Stop(v.sound, v.id);
        v.sound = nullptr;
    } else if (vehicle) {
        Game::SoundAttach(v.sound, vehicle);
    }
    v.wasActive = active;
}

void HornPlayer::UpdateLocal(u8* vehicle, u32 now)
{
    bool keyHeld, keyTapped;
    Input::ReadKeyboard(keyHeld, keyTapped);
    bool held = Input::padHeld || keyHeld;
    u32 presses = Input::padPresses;
    if ((held && !previousHeld) || presses != pressesSeen || keyTapped) {
        minUntil = now + MinHornMs;
        minArmed = true;
    }
    pressesSeen = presses;
    if (minArmed && static_cast<i32>(now - minUntil) >= 0) minArmed = false;
    previousHeld = held;
    bool active = held || minArmed;
    g_wireBit = active;
    local.volume = Config::settings.playerGain;
    Drive(local, active, vehicle, Channels::PlayerHorn, now);
}

void HornPlayer::UpdateRemote(u32 now)
{
    for (int i = 0; i < RemoteSlots; i++) {
        Voice& v = remote[i];
        bool bit = g_rx[i] != 0;
        if (!bit) {
            v.muted = false;
            if (!v.wasActive && !v.sound) continue;
        }
        if (v.muted) bit = false;
        u8* vehicle = Game::VehicleAt(i);
        if (!vehicle) bit = false;
        u32 tag = static_cast<u32>(reinterpret_cast<u64>(vehicle));
        if (tag != v.vehicleTag) {
            v.vehicleTag = tag;
            if (v.sound || v.wasActive) {
                Release(v);
                continue;
            }
        }
        if (bit) {
            if (!v.wasActive) v.startTick = now;
            else if (now - v.startTick >= MaxRemoteMs) {
                v.muted = true;
                bit = false;
            }
        }
        v.volume = Config::settings.competitorGain;
        Drive(v, bit, vehicle, Channels::CompetitorHorn, now);
    }
}

void HornPlayer::Frame(u8* trafficAudio)
{
    frameCount++;
    HornSample::gameThread = GetCurrentThreadId();
    pool = trafficAudio + Offsets::TrafficSoundPool;
    u32 now = GetTickCount();

    HornSample::busy = 1;
    u8* entry = HornSample::Select(frameCount);
    if (HornSample::stopRequest) {
        StopAll();
        HornSample::stopRequest = 0;
    }

    u8* vehicle = Game::LocalVehicle();
    if (vehicle != lastVehicle || entry != lastEntry) {
        lastVehicle = vehicle;
        lastEntry = entry;
        StopAll();
    }

    UpdateLocal(vehicle, now);
    UpdateRemote(now);

    bool any = Alive(local);
    for (int i = 0; i < RemoteSlots && !any; i++) any = Alive(remote[i]);
    if (Draining()) any = true;
    HornSample::voicesActive = any;
    HornSample::busy = 0;
}
