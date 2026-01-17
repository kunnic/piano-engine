#include "raylib.h"
#include <stdio.h>

char currentText[64] = "PRESS KEY"; 
Font font;

void PrintToScreen(const char* text)
{
    float fontSize = 100.0f;
    float spacing = 2.0f;

    Vector2 text_size = MeasureTextEx(font, text, fontSize, spacing);
    Vector2 position;

    position.x = GetScreenWidth() / 10;
    position.y = GetScreenHeight() / 10;

    DrawTextEx(font, text, position, fontSize, spacing, BLACK);
}

// --- MAIN ---
int main(void)
{
    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "My Chordie");
    
    SetTargetFPS(60);

    font = LoadFontEx("fonts/cg.ttf", 120, 0, 0);

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose())
    {
        int key = GetKeyPressed();

        if (key != 0) 
        {
            if (key == 32) {
                sprintf(currentText, "SPACE");
            } 
            else if (key >= 32 && key <= 126) {
                sprintf(currentText, "%c", (char)key);
            }
            else {
                sprintf(currentText, "KEY %d", key);
            }
        }

        // --- RENDER ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        PrintToScreen(currentText);

        EndDrawing();
    }

    UnloadFont(font);

    CloseWindow();
    return 0;
}