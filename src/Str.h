#pragma once
#include "Types.h"

namespace Str {
int Length(const char* s);
char Lower(char c);
bool EqualsI(const char* a, const char* b);
void Append(char* dst, const char* src);
void Normalize(char* dst, const char* src, int n);
bool ParseInt(const char* s, i64& out);
float ParseFloat(const char* s, float fallback);
}
