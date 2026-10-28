#include "Config.h"
#include "Str.h"
#include "Win32.h"

Settings Config::settings = { 0x48, 0x2000, 2, 25.0f, 1.5f, true, false, "70_1C_1D_00.dat" };

namespace {

struct NameValue {
    const char* name;
    u32 value;
};

const NameValue KeyNames[] = {
    { "lbutton", 0x01 }, { "rbutton", 0x02 }, { "mbutton", 0x04 }, { "xbutton1", 0x05 }, { "xbutton2", 0x06 },
    { "back", 0x08 }, { "backspace", 0x08 }, { "tab", 0x09 }, { "return", 0x0D }, { "enter", 0x0D },
    { "shift", 0x10 }, { "control", 0x11 }, { "ctrl", 0x11 }, { "menu", 0x12 }, { "alt", 0x12 },
    { "pause", 0x13 }, { "capital", 0x14 }, { "capslock", 0x14 }, { "escape", 0x1B }, { "esc", 0x1B },
    { "space", 0x20 }, { "prior", 0x21 }, { "pageup", 0x21 }, { "next", 0x22 }, { "pagedown", 0x22 },
    { "end", 0x23 }, { "home", 0x24 }, { "left", 0x25 }, { "up", 0x26 }, { "right", 0x27 }, { "down", 0x28 },
    { "insert", 0x2D }, { "delete", 0x2E }, { "multiply", 0x6A }, { "add", 0x6B }, { "subtract", 0x6D },
    { "decimal", 0x6E }, { "divide", 0x6F }, { "lshift", 0xA0 }, { "rshift", 0xA1 }, { "lcontrol", 0xA2 },
    { "rcontrol", 0xA3 }, { "lmenu", 0xA4 }, { "rmenu", 0xA5 }, { "lalt", 0xA4 }, { "ralt", 0xA5 },
    { "oem1", 0xBA }, { "oemplus", 0xBB }, { "oemcomma", 0xBC }, { "oemminus", 0xBD }, { "oemperiod", 0xBE },
    { "oem2", 0xBF }, { "oem3", 0xC0 }, { "oem4", 0xDB }, { "oem5", 0xDC }, { "oem6", 0xDD }, { "oem7", 0xDE },
    { "none", 0 }, { "off", 0 }, { nullptr, 0 }
};

const NameValue XboxNames[] = {
    { "a", 0x1000 }, { "b", 0x2000 }, { "x", 0x4000 }, { "y", 0x8000 }, { "lb", 0x0100 }, { "rb", 0x0200 },
    { "lt", Config::XboxLeftTrigger }, { "rt", Config::XboxRightTrigger }, { "back", 0x20 }, { "view", 0x20 },
    { "start", 0x10 }, { "menu", 0x10 }, { "ls", 0x40 }, { "l3", 0x40 }, { "rs", 0x80 }, { "r3", 0x80 },
    { "dpadup", 0x01 }, { "dpaddown", 0x02 }, { "dpadleft", 0x04 }, { "dpadright", 0x08 },
    { "none", 0 }, { "off", 0 }, { nullptr, 0 }
};

const NameValue DsNames[] = {
    { "square", 0 }, { "cross", 1 }, { "x", 1 }, { "circle", 2 }, { "triangle", 3 }, { "l1", 4 }, { "r1", 5 },
    { "l2", 6 }, { "r2", 7 }, { "share", 8 }, { "create", 8 }, { "options", 9 }, { "l3", 10 }, { "r3", 11 },
    { "ps", 12 }, { "touchpad", 13 }, { "mute", 14 },
    { "dpadup", Config::DsDpadUp }, { "dpadright", Config::DsDpadRight },
    { "dpaddown", Config::DsDpadDown }, { "dpadleft", Config::DsDpadLeft }, { nullptr, 0 }
};

bool Lookup(const NameValue* table, const char* s, u32& out)
{
    for (; table->name; table++) {
        if (Str::EqualsI(table->name, s)) {
            out = table->value;
            return true;
        }
    }
    return false;
}

const char DefaultIni[] =
    "; hpr_horn.asi - networked hold-to-honk player horn for NFS Hot Pursuit Remastered\r\n"
    "; Every player who should hear horns needs the mod. Set a bind to None to disable it.\r\n"
    ";\r\n"
    "; Keyboard: VK_ names (VK_H, VK_SPACE, VK_F5, VK_NUMPAD0, VK_LSHIFT, VK_XBUTTON1 ...),\r\n"
    ";           a single letter/digit, or a raw virtual-key code (72 or 0x48)\r\n"
    "; Xbox:     A B X Y LB RB LT RT Back Start LS RS DPadUp DPadDown DPadLeft DPadRight,\r\n"
    ";           or a raw XInput button mask (0x2000 = B)\r\n"
    "; DS4/DS5:  Square Cross Circle Triangle L1 R1 L2 R2 Share Options L3 R3 PS Touchpad\r\n"
    ";           DPadUp DPadDown DPadLeft DPadRight, or Button1..Button32 (Windows button number)\r\n"
    "; Gain:     horn volume multiplier, 0 - 100\r\n"
    "\r\n"
    "[HPR Horn]\r\n"
    "Keyboard Horn Hotkey = VK_H\r\n"
    "Xbox Horn Bind = B\r\n"
    "DS4/DS5 Horn Bind = Circle\r\n"
    "Player Horn Gain Level = 25.0\r\n"
    "Competitor Horn Gain Level = 1.5\r\n"
    "\r\n"
    "; Play horns from the ASI's own copy of the horn sound. Set to Off if horns go silent.\r\n"
    "Private Horn Copy = On\r\n"
    "\r\n"
    "; Experimental: play a custom EAAC SPS horn sound (e.g. a Most Wanted 2012 horn) instead of the\r\n"
    "; game's horn. The file can be a raw SPS stream or a Wave resource with its 128-byte header.\r\n"
    "; The path is relative to this folder. A sound flagged as looping keeps playing for as long as\r\n"
    "; the horn is held. Other players need the same file and setting to hear it.\r\n"
    "Custom Horn Sound = Off\r\n"
    "Custom Horn File = 70_1C_1D_00.dat\r\n";

char iniText[8192];

}

bool Config::ParseKey(const char* raw, u32& out)
{
    char s[64];
    Str::Normalize(s, raw, Str::Length(raw));
    const char* p = s;
    if (p[0] == 'v' && p[1] == 'k') p += 2;
    if (!*p) return false;
    if (!p[1] && ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))) {
        out = (*p >= 'a') ? static_cast<u32>(*p - 32) : static_cast<u32>(*p);
        return true;
    }
    i64 n;
    if (p[0] == 'f' && p[1] >= '1' && p[1] <= '9' && Str::ParseInt(p + 1, n) && n >= 1 && n <= 24) {
        out = 0x6F + static_cast<u32>(n);
        return true;
    }
    if (p[0] == 'n' && p[1] == 'u' && p[2] == 'm' && p[3] == 'p' && p[4] == 'a' && p[5] == 'd' &&
        p[6] >= '0' && p[6] <= '9' && !p[7]) {
        out = 0x60 + static_cast<u32>(p[6] - '0');
        return true;
    }
    if (Lookup(KeyNames, p, out)) return true;
    if (Str::ParseInt(p, n) && n >= 0 && n < 256) {
        out = static_cast<u32>(n);
        return true;
    }
    return false;
}

bool Config::ParseXbox(const char* raw, u32& out)
{
    char s[64];
    Str::Normalize(s, raw, Str::Length(raw));
    if (Lookup(XboxNames, s, out)) return true;
    i64 n;
    if (Str::ParseInt(s, n) && n >= 0) {
        out = static_cast<u32>(n);
        return true;
    }
    return false;
}

bool Config::ParseDs(const char* raw, i32& out)
{
    char s[64];
    Str::Normalize(s, raw, Str::Length(raw));
    if (Str::EqualsI(s, "none") || Str::EqualsI(s, "off")) {
        out = -1;
        return true;
    }
    u32 v;
    if (Lookup(DsNames, s, v)) {
        out = static_cast<i32>(v);
        return true;
    }
    const char* p = s;
    if (p[0] == 'b' && p[1] == 'u' && p[2] == 't' && p[3] == 't' && p[4] == 'o' && p[5] == 'n') p += 6;
    i64 n;
    if (Str::ParseInt(p, n) && n >= 1 && n <= 32) {
        out = static_cast<i32>(n) - 1;
        return true;
    }
    return false;
}

bool Config::ParseSwitch(const char* raw, bool& out)
{
    if (Str::EqualsI(raw, "on") || Str::EqualsI(raw, "1") || Str::EqualsI(raw, "true") || Str::EqualsI(raw, "yes")) {
        out = true;
        return true;
    }
    if (Str::EqualsI(raw, "off") || Str::EqualsI(raw, "0") || Str::EqualsI(raw, "false") || Str::EqualsI(raw, "no")) {
        out = false;
        return true;
    }
    return false;
}

void Config::Apply(const char* key, const char* value)
{
    Settings& s = settings;
    if (Str::EqualsI(key, "keyboardhornhotkey")) {
        ParseKey(value, s.keyboardKey);
    } else if (Str::EqualsI(key, "xboxhornbind")) {
        ParseXbox(value, s.xboxMask);
    } else if (Str::EqualsI(key, "ds4ds5hornbind")) {
        ParseDs(value, s.dsButton);
    } else if (Str::EqualsI(key, "playerhorngainlevel")) {
        float g = Str::ParseFloat(value, -1.0f);
        if (g >= 0.0f && g <= 100.0f) s.playerGain = g;
    } else if (Str::EqualsI(key, "competitorhorngainlevel")) {
        float g = Str::ParseFloat(value, -1.0f);
        if (g >= 0.0f && g <= 100.0f) s.competitorGain = g;
    } else if (Str::EqualsI(key, "privatehorncopy")) {
        ParseSwitch(value, s.privateCopy);
    } else if (Str::EqualsI(key, "customhornsound")) {
        ParseSwitch(value, s.customSound);
    } else if (Str::EqualsI(key, "customhornfile")) {
        int i = 0;
        for (; value[i] && i < 259; i++) s.customFile[i] = value[i];
        s.customFile[i] = 0;
    }
}

void Config::Load(const char* dir)
{
    char path[600];
    path[0] = 0;
    Str::Append(path, dir);
    Str::Append(path, "hpr_horn.ini");
    u32 n = 0;
    HANDLE f = CreateFileA(path, Win::GenericRead, 1, nullptr, 3, 0x80, nullptr);
    if (!Win::Valid(f)) {
        f = CreateFileA(path, Win::GenericWrite, 0, nullptr, 1, 0x80, nullptr);
        if (Win::Valid(f)) {
            u32 written;
            WriteFile(f, DefaultIni, sizeof(DefaultIni) - 1, &written, nullptr);
            CloseHandle(f);
        }
        for (n = 0; n < sizeof(DefaultIni) - 1; n++) iniText[n] = DefaultIni[n];
    } else {
        ReadFile(f, iniText, sizeof(iniText) - 1, &n, nullptr);
        CloseHandle(f);
    }
    iniText[n] = 0;

    char* s = iniText;
    while (*s) {
        char* line = s;
        while (*s && *s != '\n') s++;
        char* end = s;
        if (*s) s++;
        if (end > line && end[-1] == '\r') end--;
        while (line < end && (*line == ' ' || *line == '\t')) line++;
        if (line >= end || *line == ';' || *line == '#' || *line == '[') continue;
        char* eq = line;
        while (eq < end && *eq != '=') eq++;
        if (eq >= end) continue;
        char key[64];
        Str::Normalize(key, line, static_cast<int>(eq - line));
        char* v = eq + 1;
        while (v < end && (*v == ' ' || *v == '\t')) v++;
        char* ve = v;
        while (ve < end && *ve != ';') ve++;
        while (ve > v && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
        char value[260];
        int vn = static_cast<int>(ve - v);
        if (vn > 259) vn = 259;
        for (int i = 0; i < vn; i++) value[i] = v[i];
        value[vn] = 0;
        Apply(key, value);
    }
}
