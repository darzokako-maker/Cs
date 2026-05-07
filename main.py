import pygame
import win32gui
import win32con
import win32api
import numpy as np
import pymem
import pymem.process
import time
from array import array

# --- GÜNCEL OFFSETLER (30 Nisan 2026) ---
class Offsets:
    dwLocalPlayerPawn = 0x2057720
    dwEntityList = 0x24D1DF0
    dwViewMatrix = 0x2331B30
    m_pGameSceneNode = 0x318
    m_iHealth = 0x334
    m_iTeamNum = 0x3CB
    m_vOldOrigin = 0x1324

SCREEN_RES = (1920, 1080) # Kendi monitörüne göre kontrol et
TARGET_FPS = 144

def setup_overlay():
    pygame.init()
    screen = pygame.display.set_mode(SCREEN_RES, pygame.NOFRAME)
    hwnd = pygame.display.get_wm_info()['window']
    
    # Layered pencere ve tıklama geçirgenliği (WS_EX_TRANSPARENT)
    ex_style = win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE)
    win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, ex_style | win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT)
    win32gui.SetLayeredWindowAttributes(hwnd, win32api.RGB(0, 0, 0), 0, win32con.LWA_COLORKEY)
    win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, 0, 0, 0, 0, win32con.SWP_NOMOVE | win32con.SWP_NOSIZE)
    return screen

def world_to_screen(matrix, pos):
    """NumPy ile optimize edilmiş hızlı W2S"""
    w = matrix[12] * pos[0] + matrix[13] * pos[1] + matrix[14] * pos[2] + matrix[15]
    if w < 0.01: return None
    
    nx = (matrix[0] * pos[0] + matrix[1] * pos[1] + matrix[2] * pos[2] + matrix[3]) / w
    ny = (matrix[4] * pos[0] + matrix[5] * pos[1] + matrix[6] * pos[2] + matrix[7]) / w
    
    x = (SCREEN_RES[0] / 2 * nx) + (nx + SCREEN_RES[0] / 2)
    y = -(SCREEN_RES[1] / 2 * ny) + (ny + SCREEN_RES[1] / 2)
    return int(x), int(y)

def main():
    try:
        pm = pymem.Pymem("cs2.exe")
        client = pymem.process.module_from_name(pm.process_handle, "client.dll").lpBaseOfDll
        overlay = setup_overlay()
        clock = pygame.time.Clock()
    except Exception as e:
        print(f"Hata: {e}"); return

    while True:
        overlay.fill((0, 0, 0)) # Ekranı temizle
        
        try:
            # ViewMatrix Oku
            v_matrix = array('f', pm.read_bytes(client + Offsets.dwViewMatrix, 64))
            
            local_pawn = pm.read_longlong(client + Offsets.dwLocalPlayerPawn)
            entity_list = pm.read_longlong(client + Offsets.dwEntityList)
            local_team = pm.read_int(local_pawn + Offsets.m_iTeamNum)

            for i in range(1, 64):
                # Entity Listesi Okuma Mantığı
                entry = pm.read_longlong(entity_list + (8 * (i & 0x7FFF) >> 9) + 16)
                if not entry: continue
                pawn = pm.read_longlong(entry + 120 * (i & 0x1FF))
                if not pawn or pawn == local_pawn: continue
                
                # Can ve Takım Kontrolü
                health = pm.read_int(pawn + Offsets.m_iHealth)
                if health <= 0 or health > 100: continue
                
                team = pm.read_int(pawn + Offsets.m_iTeamNum)
                color = (255, 0, 0) if team != local_team else (0, 255, 0)

                # Pozisyon Hesapla (Ayak ve Kafa)
                origin = [pm.read_float(pawn + Offsets.m_vOldOrigin + j*4) for j in range(3)]
                foot = world_to_screen(v_matrix, origin)
                head = world_to_screen(v_matrix, [origin[0], origin[1], origin[2] + 72.0])

                if foot and head:
                    h = foot[1] - head[1]
                    w = h / 2
                    # Box Çizimi
                    pygame.draw.rect(overlay, color, (head[0] - w/2, head[1], w, h), 2)
                    
        except: pass

        pygame.display.update()
        clock.tick(TARGET_FPS)

if __name__ == "__main__":
    main()
    
