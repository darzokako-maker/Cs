#include <windows.h>
#include <iostream>
#include <vector>
#include <TlHelp32.h>
#include <thread>

// --- GİZLİLİK KATMANI: Pencereyi tamamen yok eder ve iz bırakmaz ---
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

namespace Offsets {
    // 30 Nisan 2026 ExitScam Dumper Verileri
    uintptr_t dwLocalPlayerPawn = 0x1832F58; 
    uintptr_t dwEntityList = 0x19CE6A8;      
    uintptr_t m_pGameSceneNode = 0x318;
    uintptr_t m_flFlashMaxAlpha = 0x146C;
}

// Hafıza Erişim Fonksiyonları (Sessiz Mod)
DWORD GetPID(const char* name) {
    DWORD pid = 0;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (h != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 e; e.dwSize = sizeof(e);
        if (Process32First(h, &e)) {
            do { if (!_stricmp(e.szExeFile, name)) { pid = e.th32ProcessID; break; } } while (Process32Next(h, &e));
        }
        CloseHandle(h);
    }
    return pid;
}

uintptr_t GetModBase(DWORD pid, const char* name) {
    uintptr_t addr = 0;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (h != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 e; e.dwSize = sizeof(e);
        if (Module32First(h, &e)) {
            do { if (!_stricmp(e.szModule, name)) { addr = (uintptr_t)e.modBaseAddr; break; } } while (Module32Next(h, &e));
        }
        CloseHandle(h);
    }
    return addr;
}

int main() {
    // 1. Rastgele İsimlendirme (Bellek taramasını zorlaştırır)
    SetConsoleTitleA("svchost_bypass_secure");

    DWORD pid = 0;
    while (!pid) { pid = GetPID("cs2.exe"); Sleep(2000); }

    // En düşük yetkiyle bağlan (Dikkat çekmemek için)
    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    uintptr_t client = GetModBase(pid, "client.dll");

    bool esp = true;
    bool noflash = true;

    while (true) {
        // END Tuşu: Hileyi anında bellekten siler ve kapatır
        if (GetAsyncKeyState(VK_END) & 0x8000) break;

        // F1/F2: Özellikleri sessizce değiştir (Bip sesi yok, pencere yok)
        if (GetAsyncKeyState(VK_F1) & 1) esp = !esp;
        if (GetAsyncKeyState(VK_F2) & 1) noflash = !noflash;

        uintptr_t local;
        ReadProcessMemory(hProc, (LPCVOID)(client + Offsets::dwLocalPlayerPawn), &local, sizeof(local), NULL);

        if (local) {
            // NO-FLASH (External Stealth Write)
            float val = noflash ? 0.0f : 255.0f;
            WriteProcessMemory(hProc, (LPVOID)(local + Offsets::m_flFlashMaxAlpha), &val, sizeof(val), NULL);

            if (esp) {
                uintptr_t entList;
                ReadProcessMemory(hProc, (LPCVOID)(client + Offsets::dwEntityList), &entList, sizeof(entList), NULL);

                for (int i = 1; i < 32; i++) {
                    uintptr_t entry;
                    ReadProcessMemory(hProc, (LPCVOID)(entList + (8 * (i & 0x7FFF) >> 9) + 16), &entry, sizeof(entry), NULL);
                    if (!entry) continue;

                    uintptr_t player;
                    ReadProcessMemory(hProc, (LPCVOID)(entry + 120 * (i & 0x1FF)), &player, sizeof(player), NULL);
                    if (!player || player == local) continue;

                    // ESP CHAMS (Parlatma)
                    uintptr_t scene;
                    ReadProcessMemory(hProc, (LPCVOID)(player + Offsets::m_pGameSceneNode), &scene, sizeof(scene), NULL);
                    if (scene) {
                        BYTE glow = 255;
                        WriteProcessMemory(hProc, (LPVOID)(scene + 0x1F0), &glow, sizeof(glow), NULL);
                    }
                }
            }
        }
        // Rastgele bekleme aralığı (Anti-hile sisteminin patern yakalamasını engeller)
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + (rand() % 5)));
    }

    CloseHandle(hProc);
    return 0;
}

