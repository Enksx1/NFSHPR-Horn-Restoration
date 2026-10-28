#include "Config.h"
#include "Game.h"
#include "Hooks.h"
#include "HornPlayer.h"
#include "HornSample.h"
#include "Input.h"
#include "Shared.h"
#include "Win32.h"

extern "C" {
u64 g_frame_orig;
u64 g_n2_call;
u64 g_n1_skip;
u64 g_n2_skip;
volatile i64 g_localSlotOff = -1;
volatile i32 g_wireBit;
volatile u8 g_rx[32];

void HornFrame(u8* trafficAudio)
{
    HornPlayer::Frame(trafficAudio);
}

void OnEntryCreated(u8* entry)
{
    HornSample::OnCreated(entry);
}

void OnEntryUnload(u8* entry)
{
    HornSample::OnUnload(entry);
}
}

namespace {

char moduleDir[512];

u32 __stdcall MainThread(void*)
{
    Game::Init();
    Config::Load(moduleDir);
    HornSample::LoadCustom(moduleDir);
    Hooks::Result result = Hooks::Changed;
    for (int attempt = 0; attempt < 5 && result == Hooks::Changed; attempt++) {
        while (!Hooks::SitesReady()) Sleep(250);
        if (attempt == 0) Input::Init();
        result = Hooks::Install();
        if (result == Hooks::Changed) Sleep(500);
    }
    if (result != Hooks::Installed) return 0;
    Input::StartControllerThread();
    HornSample::FindLoaded();
    return 0;
}

}

extern "C" BOOL __stdcall DllMain(HANDLE self, u32 reason, void*)
{
    if (reason == 1) {
        DisableThreadLibraryCalls(self);
        u32 n = GetModuleFileNameA(self, moduleDir, sizeof(moduleDir) - 32);
        while (n && moduleDir[n - 1] != '\\' && moduleDir[n - 1] != '/') n--;
        moduleDir[n] = 0;
        HANDLE t = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return 1;
}
