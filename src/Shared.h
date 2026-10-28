#pragma once
#include "Types.h"

extern "C" {
extern u64 g_frame_orig;
extern u64 g_n2_call;
extern u64 g_n1_skip;
extern u64 g_n2_skip;
extern volatile i64 g_localSlotOff;
extern volatile i32 g_wireBit;
extern volatile u8 g_rx[32];
}
