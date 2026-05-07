import pymem
import pymem.process
import pygame
import keyboard
import time
from array import array

# 30 Nisan 2026 - Dosyalarından Çekilen Güncel Adresler
class Offsets:
    dwLocalPlayerPawn = 0x2057720
    dwEntityList = 0x24D1DF0
    dwViewMatrix = 0x2331B30
    m_pGameSceneNode = 0x318
    m_iHealth = 0x334
    m_iTeamNum = 0x3CB
    m_vOldOrigin = 0x1324 

# Ekran Ayarları
WIDTH, HEIGHT = 1920, 1080 

def world_to_screen(matrix, pos):
    w = matrix[12] * pos[0] + matrix[13] * pos[1] + matrix[14] * pos[2] + matrix[15]
    if w < 0.01: return None
    nx = (matrix[0] * pos[0] + matrix[1] * pos[1] + matrix[2] * pos[2] + matrix[3]) / w
    ny = (matrix[4] * pos[0] + matrix[5] * pos[1] + matrix[6] * pos[2] + matrix[7]) / w
    return int((WIDTH / 2 * nx) + (nx + WIDTH / 2)), int(-(HEIGHT / 2 * ny) + (ny + HEIGHT / 2))

def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH, HEIGHT), pygame.NOFRAME)
    
    # Windows Şeffaflık Ayarları
    import win32api, win32con, win32gui
    hwnd = win32gui.GetForegroundWindow()
    win32gui.SetWindowLong(hwnd, win32con.GWL_EXSTYLE, win32gui.GetWindowLong(hwnd, win32con.GWL_EXSTYLE) | win32con.WS_EX_LAYERED | win32con.WS_EX_TRANSPARENT)
    win32gui.SetLayeredWindowAttributes(hwnd, win32api.RGB(0, 0, 0), 0, win32con.LWA_COLORKEY)
    win32gui.SetWindowPos(hwnd, win32con.HWND_TOPMOST, 0, 0, 0, 0, win32con.SWP_NOMOVE | win32con.SWP_NOSIZE)

    try:
        pm = pymem.Pymem("cs2.exe")
        client = pymem.process.module_from_name(pm.process_handle, "client.dll").lpBaseOfDll
        print(">> CS2 External Active")
    except:
        print(">> CS2 Not Found!"); return

    while not keyboard.is_pressed('end'):
        screen.fill((0, 0, 0))
        try:
            matrix = array('f', pm.read_bytes(client + Offsets.dwViewMatrix, 64))
            lp = pm.read_longlong(client + Offsets.dwLocalPlayerPawn)
            elist = pm.read_longlong(client + Offsets.dwEntityList)
            lteam = pm.read_int(lp + Offsets.m_iTeamNum)

            for i in range(1, 64):
                entry = pm.read_longlong(elist + (8 * (i & 0x7FFF) >> 9) + 16)
                if not entry: continue
                pawn = pm.read_longlong(entry + 120 * (i & 0x1FF))
                if not pawn or pawn == lp: continue
                
                health = pm.read_int(pawn + Offsets.m_iHealth)
                if health <= 0 or health > 100: continue
                
                team = pm.read_int(pawn + Offsets.m_iTeamNum)
                color = (255, 70, 70) if team != lteam else (70, 255, 70)

                origin = [pm.read_float(pawn + Offsets.m_vOldOrigin + j*4) for j in range(3)]
                f_scr = world_to_screen(matrix, origin)
                h_scr = world_to_screen(matrix, [origin[0], origin[1], origin[2] + 72.0])

                if f_scr and h_scr:
                    h = f_scr[1] - h_scr[1]
                    w = h / 2
                    pygame.draw.rect(screen, color, (h_scr[0] - w/2, h_scr[1], w, h), 2)
                    pygame.draw.line(screen, (0, 255, 0), (h_scr[0] - w/2 - 5, f_scr[1]), (h_scr[0] - w/2 - 5, f_scr[1] - (h * health / 100)), 2)
        except: pass
        pygame.display.update()
        time.sleep(0.01)
    pygame.quit()

if __name__ == "__main__":
    main()
