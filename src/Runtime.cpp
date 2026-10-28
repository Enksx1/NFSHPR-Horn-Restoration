#include "Types.h"
#include <intrin.h>

extern "C" {
int _fltused = 0;

#pragma function(memcpy, memset, memcmp)

void* memcpy(void* dst, const void* src, size_t n)
{
    __movsb(static_cast<unsigned char*>(dst), static_cast<const unsigned char*>(src), n);
    return dst;
}

void* memset(void* dst, int value, size_t n)
{
    __stosb(static_cast<unsigned char*>(dst), static_cast<unsigned char>(value), n);
    return dst;
}

int memcmp(const void* a, const void* b, size_t n)
{
    const volatile u8* x = static_cast<const u8*>(a);
    const volatile u8* y = static_cast<const u8*>(b);
    for (size_t i = 0; i < n; i++) {
        if (x[i] != y[i]) return x[i] - y[i];
    }
    return 0;
}
}
