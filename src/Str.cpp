#include "Str.h"

namespace Str {

int Length(const char* s)
{
    int n = 0;
    while (s[n]) n++;
    return n;
}

char Lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c;
}

bool EqualsI(const char* a, const char* b)
{
    while (*a && *b) {
        if (Lower(*a) != Lower(*b)) return false;
        a++;
        b++;
    }
    return *a == *b;
}

void Append(char* dst, const char* src)
{
    dst += Length(dst);
    while ((*dst++ = *src++)) {}
}

void Normalize(char* dst, const char* src, int n)
{
    int j = 0;
    for (int i = 0; i < n && j < 63; i++) {
        char c = Lower(src[i]);
        if (c == ' ' || c == '\t' || c == '_' || c == '/' || c == '-') continue;
        dst[j++] = c;
    }
    dst[j] = 0;
}

bool ParseInt(const char* s, i64& out)
{
    i64 v = 0;
    bool any = false;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        for (s += 2;; s++) {
            int d;
            char c = Lower(*s);
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else break;
            v = v * 16 + d;
            any = true;
        }
    } else {
        for (; *s >= '0' && *s <= '9'; s++) {
            v = v * 10 + (*s - '0');
            any = true;
        }
    }
    if (!any || *s) return false;
    out = v;
    return true;
}

float ParseFloat(const char* s, float fallback)
{
    float v = 0.0f, scale = 1.0f;
    bool neg = false, any = false;
    if (*s == '-') {
        neg = true;
        s++;
    }
    for (; *s >= '0' && *s <= '9'; s++) {
        v = v * 10.0f + static_cast<float>(*s - '0');
        any = true;
    }
    if (*s == '.') {
        for (s++; *s >= '0' && *s <= '9'; s++) {
            scale /= 10.0f;
            v += static_cast<float>(*s - '0') * scale;
            any = true;
        }
    }
    if (!any || *s) return fallback;
    return neg ? -v : v;
}

}
