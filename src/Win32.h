#pragma once
#include "Types.h"

using HANDLE = void*;
using BOOL = int;
using ThreadProc = u32(__stdcall*)(void*);

struct MemoryInfo {
    void* BaseAddress;
    void* AllocationBase;
    u32 AllocationProtect;
    u16 PartitionId;
    u16 Reserved0;
    u64 RegionSize;
    u32 State;
    u32 Protect;
    u32 Type;
    u32 Reserved1;
};

extern "C" {
__declspec(dllimport) HANDLE __stdcall GetModuleHandleA(const char*);
__declspec(dllimport) u32 __stdcall GetModuleFileNameA(HANDLE, char*, u32);
__declspec(dllimport) HANDLE __stdcall CreateThread(void*, u64, ThreadProc, void*, u32, u32*);
__declspec(dllimport) void __stdcall Sleep(u32);
__declspec(dllimport) u32 __stdcall GetTickCount();
__declspec(dllimport) void* __stdcall VirtualAlloc(void*, u64, u32, u32);
__declspec(dllimport) BOOL __stdcall VirtualFree(void*, u64, u32);
__declspec(dllimport) BOOL __stdcall VirtualProtect(void*, u64, u32, u32*);
__declspec(dllimport) u64 __stdcall VirtualQuery(const void*, MemoryInfo*, u64);
__declspec(dllimport) BOOL __stdcall FlushInstructionCache(HANDLE, const void*, u64);
__declspec(dllimport) HANDLE __stdcall GetCurrentProcess();
__declspec(dllimport) BOOL __stdcall ReadProcessMemory(HANDLE, const void*, void*, u64, u64*);
__declspec(dllimport) u32 __stdcall GetCurrentProcessId();
__declspec(dllimport) u32 __stdcall GetCurrentThreadId();
__declspec(dllimport) HANDLE __stdcall CreateFileA(const char*, u32, u32, void*, u32, u32, HANDLE);
__declspec(dllimport) BOOL __stdcall WriteFile(HANDLE, const void*, u32, u32*, void*);
__declspec(dllimport) BOOL __stdcall ReadFile(HANDLE, void*, u32, u32*, void*);
__declspec(dllimport) BOOL __stdcall CloseHandle(HANDLE);
__declspec(dllimport) HANDLE __stdcall LoadLibraryA(const char*);
__declspec(dllimport) void* __stdcall GetProcAddress(HANDLE, const char*);
__declspec(dllimport) BOOL __stdcall DisableThreadLibraryCalls(HANDLE);
}

namespace Win {
inline bool Valid(HANDLE h) { return reinterpret_cast<i64>(h) != -1; }
constexpr u32 MemCommitReserve = 0x3000;
constexpr u32 MemRelease = 0x8000;
constexpr u32 MemCommit = 0x1000;
constexpr u32 PageReadWrite = 0x04;
constexpr u32 PageExecuteRead = 0x20;
constexpr u32 PageExecuteReadWrite = 0x40;
constexpr u32 GenericRead = 0x80000000;
constexpr u32 GenericWrite = 0x40000000;
constexpr u32 StillActive = 259;
constexpr u32 Failed = 0xFFFFFFFF;
}
