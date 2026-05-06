#include <windows.h>
#include <iostream>
#include <thread>
#include <TlHelp32.h>

// --- AYARLAR ---
namespace Settings {
    bool esp = true;
    bool noflash = true;
}

namespace Offsets {
    // 30 Nisan 2026 Build 14158 Verileri
    uintptr_t dwLocalPlayerPawn = 0x1832F58; 
    uintptr_t dwEntityList = 0x19CE6A8;      
    uintptr_t m_pGameSceneNode = 0x318;
    uintptr_t m_flFlashMaxAlpha = 0x146C;
}

// Yardımcı Fonksiyonlar
uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry; modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_stricmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

void PrintMenu() {
    system("cls");
    printf("=== MEINE STEALTH V5 (DEBUG MODE) ===\n");
    printf("------------------------------------\n");
    printf("[F1] ESP (Chams)   : %s\n", Settings::esp ? "ACIK" : "KAPALI");
    printf("[F2] No-Flash      : %s\n", Settings::noflash ? "ACIK" : "KAPALI");
    printf("------------------------------------\n");
    printf("[END] Hileyi Kapat\n");
}

int main() {
    // Konsol başlığı
    SetConsoleTitleA("Intel Audio Driver - Debug Console");

    HWND hwnd = FindWindowA(NULL, "Counter-Strike 2");
    if (!hwnd) {
        printf("CS2 Bekleniyor...\n");
        while (!hwnd) {
            hwnd = FindWindowA(NULL, "Counter-Strike 2");
            Sleep(1000);
        }
    }

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    uintptr_t client = GetModuleBaseAddress(pid, "client.dll");

    PrintMenu();

    while (!GetAsyncKeyState(VK_END)) {
        // Tuş Kontrolleri
        if (GetAsyncKeyState(VK_F1) & 1) { Settings::esp = !Settings::esp; PrintMenu(); }
        if (GetAsyncKeyState(VK_F2) & 1) { Settings::noflash = !Settings::noflash; PrintMenu(); }

        uintptr_t local;
        ReadProcessMemory(hProc, (LPCVOID)(client + Offsets::dwLocalPlayerPawn), &local, sizeof(local), NULL);

        if (local) {
            // NO-FLASH
            if (Settings::noflash) {
                float flashVal = 0.0f;
                WriteProcessMemory(hProc, (LPVOID)(local + Offsets::m_flFlashMaxAlpha), &flashVal, sizeof(flashVal), NULL);
            }

            // ESP CHAMS
            if (Settings::esp) {
                uintptr_t entList;
                ReadProcessMemory(hProc, (LPCVOID)(client + Offsets::dwEntityList), &entList, sizeof(entList), NULL);

                for (int i = 1; i < 64; i++) {
                    uintptr_t entry;
                    ReadProcessMemory(hProc, (LPCVOID)(entList + (8 * (i & 0x7FFF) >> 9) + 16), &entry, sizeof(entry), NULL);
                    if (!entry) continue;

                    uintptr_t player;
                    ReadProcessMemory(hProc, (LPCVOID)(entry + 120 * (i & 0x1FF)), &player, sizeof(player), NULL);
                    if (!player || player == local) continue;

                    uintptr_t scene;
                    ReadProcessMemory(hProc, (LPCVOID)(player + Offsets::m_pGameSceneNode), &scene, sizeof(scene), NULL);
                    if (scene) {
                        BYTE glow = 255; // Parlama aktif
                        WriteProcessMemory(hProc, (LPVOID)(scene + 0x1F0), &glow, sizeof(glow), NULL);
                    }
                }
            }
        }
        Sleep(10); // İşlemciyi yormaz
    }

    CloseHandle(hProc);
    return 0;
}
