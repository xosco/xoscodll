#include "gl_proxy.h"
#include <GL/gl.h>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <string>
#include <iostream>

extern "C" {

typedef void (WINAPI* PFN_glEnable)(GLenum cap);
typedef void (WINAPI* PFN_glDisable)(GLenum cap);
typedef void (WINAPI* PFN_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void* indices);
typedef void (WINAPI* PFN_glDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices);
typedef void (WINAPI* PFN_glDrawArrays)(GLenum mode, GLint first, GLsizei count);
typedef void (WINAPI* PFN_glColor3f)(GLfloat red, GLfloat green, GLfloat blue);
typedef void (WINAPI* PFN_glPolygonMode)(GLenum face, GLenum mode);
typedef void (WINAPI* PFN_glLineWidth)(GLfloat width);
typedef void (WINAPI* PFN_glPushAttrib)(GLbitfield mask);
typedef void (WINAPI* PFN_glPopAttrib)(void);
typedef PROC (WINAPI* PFN_wglGetProcAddress)(LPCSTR name);

HMODULE g_realGL = nullptr;
static PFN_glEnable real_glEnable = nullptr;
static PFN_glDisable real_glDisable = nullptr;
static PFN_glDrawElements real_glDrawElements = nullptr;
static PFN_glDrawRangeElements real_glDrawRangeElements = nullptr;
static PFN_glDrawArrays real_glDrawArrays = nullptr;
static PFN_glColor3f real_glColor3f = nullptr;
static PFN_glPolygonMode real_glPolygonMode = nullptr;
static PFN_glLineWidth real_glLineWidth = nullptr;
static PFN_glPushAttrib real_glPushAttrib = nullptr;
static PFN_glPopAttrib real_glPopAttrib = nullptr;
static PFN_wglGetProcAddress real_wglGetProcAddress = nullptr;

static bool g_cheatsEnabled = false;
static int g_toggleKey = VK_F2;
static bool g_toggleKeyPressed = false;
static std::string g_toggleKeyName = "F2";

static std::string NormalizeKeyName(const std::string& key) {
    std::string result;
    for (char c : key) {
        if (!std::isspace((unsigned char)c)) result.push_back((char)std::toupper((unsigned char)c));
    }
    return result;
}

static int ParseToggleKey(const std::string& input) {
    std::string key = NormalizeKeyName(input);
    if (key.empty()) return 0;
    if (key.size() == 1) {
        char c = key[0];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            return VkKeyScanA(c) & 0xFF;
        }
    }
    if (key[0] == 'F' && key.size() <= 3) {
        int num = std::stoi(key.substr(1));
        if (num >= 1 && num <= 12) return VK_F1 + (num - 1);
    }
    if (key == "SPACE") return VK_SPACE;
    if (key == "ENTER" || key == "RETURN") return VK_RETURN;
    if (key == "TAB") return VK_TAB;
    if (key == "ESC" || key == "ESCAPE") return VK_ESCAPE;
    if (key == "UP") return VK_UP;
    if (key == "DOWN") return VK_DOWN;
    if (key == "LEFT") return VK_LEFT;
    if (key == "RIGHT") return VK_RIGHT;
    return 0;
}

static std::string GetToggleKeyName(int vk) {
    if (vk >= VK_F1 && vk <= VK_F12) {
        return "F" + std::to_string(vk - VK_F1 + 1);
    }
    if (vk >= 'A' && vk <= 'Z') {
        return std::string(1, (char)vk);
    }
    if (vk >= '0' && vk <= '9') {
        return std::string(1, (char)vk);
    }
    switch (vk) {
        case VK_SPACE: return "SPACE";
        case VK_RETURN: return "ENTER";
        case VK_TAB: return "TAB";
        case VK_ESCAPE: return "ESC";
        case VK_UP: return "UP";
        case VK_DOWN: return "DOWN";
        case VK_LEFT: return "LEFT";
        case VK_RIGHT: return "RIGHT";
        default: return "KEY";
    }
}

static DWORD WINAPI BinderThread(LPVOID) {
    std::string input;
    std::cout << "Enter cheat toggle key (e.g. F2, A, 1) and press Enter: ";
    std::getline(std::cin, input);
    int vk = ParseToggleKey(input);
    if (vk == 0) {
        std::cout << "Invalid key, defaulting to F2." << std::endl;
        vk = VK_F2;
        g_toggleKeyName = "F2";
    } else {
        g_toggleKeyName = GetToggleKeyName(vk);
    }
    g_toggleKey = vk;
    std::cout << "Cheat toggle bound to " << g_toggleKeyName << ". Press that key to toggle." << std::endl;
    return 0;
}

static void UpdateCheatToggle() {
    if (g_toggleKey == 0) return;
    short state = GetAsyncKeyState(g_toggleKey);
    bool pressed = (state & 0x8000) != 0;
    if (pressed && !g_toggleKeyPressed) {
        g_toggleKeyPressed = true;
        g_cheatsEnabled = !g_cheatsEnabled;
        std::cout << "Cheats " << (g_cheatsEnabled ? "ENABLED" : "DISABLED") << " (" << g_toggleKeyName << ")" << std::endl;
    } else if (!pressed) {
        g_toggleKeyPressed = false;
    }
}

void CreateDebugConsole() {
    if (AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        std::cout << "=== STANDOFF 2 SURGICAL WH ===" << std::endl;
        std::cout << "Enter cheat toggle key in the console when prompted." << std::endl;
        HANDLE thread = CreateThread(nullptr, 0, BinderThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
}

bool InitRealGL() {
    if (g_realGL) return true;
    CreateDebugConsole();
    char sysPath[MAX_PATH]; GetSystemDirectoryA(sysPath, MAX_PATH);
    std::string dllPath = std::string(sysPath) + "\\opengl32.dll";
    g_realGL = LoadLibraryA(dllPath.c_str());
    if (g_realGL) {
        real_wglGetProcAddress = (PFN_wglGetProcAddress)GetProcAddress(g_realGL, "wglGetProcAddress");
        real_glEnable = (PFN_glEnable)GetProcAddress(g_realGL, "glEnable");
        real_glDisable = (PFN_glDisable)GetProcAddress(g_realGL, "glDisable");
        real_glDrawElements = (PFN_glDrawElements)GetProcAddress(g_realGL, "glDrawElements");
        real_glDrawRangeElements = (PFN_glDrawRangeElements)GetProcAddress(g_realGL, "glDrawRangeElements");
        real_glDrawArrays = (PFN_glDrawArrays)GetProcAddress(g_realGL, "glDrawArrays");
        real_glColor3f = (PFN_glColor3f)GetProcAddress(g_realGL, "glColor3f");
        real_glPolygonMode = (PFN_glPolygonMode)GetProcAddress(g_realGL, "glPolygonMode");
        real_glLineWidth = (PFN_glLineWidth)GetProcAddress(g_realGL, "glLineWidth");
        real_glPushAttrib = (PFN_glPushAttrib)GetProcAddress(g_realGL, "glPushAttrib");
        real_glPopAttrib = (PFN_glPopAttrib)GetProcAddress(g_realGL, "glPopAttrib");
    }
    return (g_realGL != nullptr);
}

void* GetRealGLProc(const char* name) {
    if (!g_realGL) InitRealGL();
    return (void*)GetProcAddress(g_realGL, name);
}

// ГЛАВНЫЙ ФИЛЬТР (Что мы хотим видеть сквозь стены)
// Список Count/indexcount, которые рисуются через стену.
bool IsPlayer(int count) {
    if (!g_cheatsEnabled) return false;
    switch (count) {
        case 29616:
        case 37935:
        case 33918:
        case 43383:
        case 34518:
        case 35187:
        case 44046:
        case 44007:
        case 30300:
        case 39333:
            return true;
        default:
            return false;
    }
}

void Log(const char* fmt, ...) {
    // Оставляем пустым чтобы не тормозило
}

// Оптимизированная отрисовка
void WINAPI my_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    UpdateCheatToggle();
    if (!real_glDrawElements) real_glDrawElements = (PFN_glDrawElements)GetRealGLProc("glDrawElements");

    if (IsPlayer(count)) {
        real_glDisable(GL_DEPTH_TEST);
        real_glDrawElements(mode, count, type, indices);
        real_glEnable(GL_DEPTH_TEST);
    } else {
        real_glDrawElements(mode, count, type, indices);
    }
}

void WINAPI my_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) {
    UpdateCheatToggle();
    if (!real_glDrawRangeElements) real_glDrawRangeElements = (PFN_glDrawRangeElements)GetRealGLProc("glDrawRangeElements");

    if (IsPlayer(count)) {
        real_glDisable(GL_DEPTH_TEST);
        real_glDrawRangeElements(mode, start, end, count, type, indices);
        real_glEnable(GL_DEPTH_TEST);
    } else {
        real_glDrawRangeElements(mode, start, end, count, type, indices);
    }
}

void WINAPI my_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (!real_glDrawArrays) real_glDrawArrays = (PFN_glDrawArrays)GetRealGLProc("glDrawArrays");
    // Игроки редко рисуются через Arrays, поэтому тут просто оригинал
    real_glDrawArrays(mode, first, count);
}

PROC WINAPI my_wglGetProcAddress(LPCSTR name) {
    if (!real_wglGetProcAddress) InitRealGL();
    std::string n(name);
    if (n == "glDrawElements") return (PROC)my_glDrawElements;
    if (n == "glDrawRangeElements") return (PROC)my_glDrawRangeElements;
    if (n == "glDrawArrays") return (PROC)my_glDrawArrays;
    return real_wglGetProcAddress(name);
}

void ShutdownRealGL() { if (g_realGL) FreeLibrary(g_realGL); }

} // extern "C"