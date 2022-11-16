#include <Windows.h>
#include <iostream>

DWORD entityListStart = 0x0343CB50; 
DWORD viewMatrix = 0x0494B100; 
DWORD healthOffset = 0x194; 
DWORD Width = 0x00C22FE0;
DWORD Height = 0x00C22FE4;
HWND hwndBlackOps;
HBRUSH Brush;
HDC hdcBlackOps;
HFONT Font;
float Matrix[16];
COLORREF TextCOLOR;
COLORREF TextCOLORRED;

//Vec
struct Vec2
{
    float x, y;
};

struct Vec3
{
    float x, y, z;
};

struct Vec4
{
    float x, y, z, w;
};

//Drawing function
void DrawFilledRect(int x, int y, int w, int h)
{
    RECT rect = { x, y, x + w, y + h };
    FillRect(hdcBlackOps, &rect, Brush);
}
void DrawBorderBox(int x, int y, int w, int h, int thickness)
{
    DrawFilledRect(x, y, w, thickness);
    DrawFilledRect(x, y, thickness, h);
    DrawFilledRect((x + w), y, thickness, h);
    DrawFilledRect(x, y + h, w + thickness, thickness);
}
void DrawLine(int targetX, int targetY)
{
    MoveToEx(hdcBlackOps, 0, 0, NULL);
    LineTo(hdcBlackOps, targetX, targetY);

}
void DrawString(int x, int y, COLORREF color, const char* text)
{
    SetTextAlign(hdcBlackOps, TA_CENTER | TA_NOUPDATECP);
    SetBkColor(hdcBlackOps, RGB(0, 0, 0));
    SetBkMode(hdcBlackOps, TRANSPARENT);
    SetTextColor(hdcBlackOps, color);
    SelectObject(hdcBlackOps, Font);
    TextOutA(hdcBlackOps, x, y, text, strlen(text));
    DeleteObject(Font);
}

//W2S
bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], int windowWidth, int windowHeight) // 3D to 2D
{
    //Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
    Vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
    clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

    if (clipCoords.w < 0.1f)
        return false;

    //perspective division, dividing by clip.W = Normalized Device Coordinates
    Vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    //Transform to window coordinates
    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}

DWORD WINAPI HackThread(HMODULE hModule)
{
    hwndBlackOps = FindWindow(0, (L"Call of Duty®: BlackOps"));

    while (true)
    {
        memcpy(&Matrix, (PBYTE*)(viewMatrix), sizeof(Matrix));
        hdcBlackOps = GetDC(hwndBlackOps);
        //Base of player
        Vec2 vScreen;
        //Head Enemy
        Vec2 vHead;

        //our entity list loop
        for (short int i = 0; i < 64; i++)
        {
            //Create the entity
            DWORD entity = *(DWORD*)(entityListStart + 0x90 * i);

            if (entity != NULL)
            {

                //The EnemyPos 
                float enemyX = *(float*)(entity + 0x18);
                float enemyY = *(float*)(entity + 0x1C);
                float enemyZ = *(float*)(entity + 0x20);
                //Transform to Vec3
                Vec3 enemyPos = { enemyX, enemyY, enemyZ };

                //EntityHealh Addr
                DWORD health = *(DWORD*)(entity + healthOffset);
                float trueWidth = *(float*)(Width);
                float trueHeight = *(float*)(Height);

                //Verify if the enemy is still alive
                if (health > 0) {
                    //3D to 2D
                    if (WorldToScreen(enemyPos, vScreen, Matrix, trueWidth, trueHeight))
                    {
                        //The EnemyHeadPos
                        float enemyHeadX = *(float*)(entity + 0x118);
                        float enemyHeadY = *(float*)(entity + 0x11C);
                        float enemyHeadZ = *(float*)(entity + 0x120);
                        //Transform to Vec3
                        Vec3 enemyHeadPos = { enemyHeadX, enemyHeadY, enemyHeadZ };

                        //3D to 2D
                        if (WorldToScreen(enemyHeadPos, vHead, Matrix, trueWidth, trueHeight)) {
                            //Calcul permettant placer la box
                            float head = vHead.y - vScreen.y;
                            float width = head / 2;
                            float center = width / -2;

                            //Draw the ESPline
                            DrawLine(vScreen.x, vScreen.y);

                            //Color of the ESP Box
                            Brush = CreateSolidBrush(RGB(0, 255, 0));
                            //Draw the ESP Box
                            DrawBorderBox(vScreen.x + center, vScreen.y, width, head - 5, 1);
                            //To delete the box 
                            DeleteObject(Brush);
                        }
                    }
                }
            }
        }
        // Eject DLL
        if (GetAsyncKeyState(VK_END) & 1)
        {
            break;
        }
        Sleep(1);
        DeleteObject(hdcBlackOps);
    }
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
