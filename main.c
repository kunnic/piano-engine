#if defined(_WIN32)
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
//#define NONLS           // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

typedef struct tagMSG *LPMSG;

#include <windows.h>

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#endif

#undef PlaySound

#include "raylib.h"
#include <stdio.h>
#include <string.h>

char infoBuffer[2048] = "Scanning..."; 
Font font;

// for scanning MIDI devices
void ScanMidiDevicesToBuffer()
{
    infoBuffer[0] = '\0'; 
    
    unsigned int deviceCount = midiInGetNumDevs();

    char tempLine[256];
    sprintf(tempLine, "FOUND %d MIDI DEVICES:\n--------------------\n", deviceCount);
    strcat(infoBuffer, tempLine);

    for (unsigned int i = 0; i < deviceCount; i++) {
        MIDIINCAPS caps;
        if (midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS)) == MMSYSERR_NOERROR) {
            sprintf(tempLine, "[ID %d]: %s\n", i, caps.szPname);
            strcat(infoBuffer, tempLine);
        }
    }

    if (deviceCount == 0) {
        strcat(infoBuffer, "(No MIDI device found.)\n(Try plugging in USB & Press R)");
    }
}

void PrintToScreen(const char* text)
{
    float fontSize = 50.0f; 
    float spacing = 2.0f;
    Vector2 position = { 50.0f, 50.0f };

    DrawTextEx(font, text, position, fontSize, spacing, BLACK);
}

int main(void)
{
    SetTraceLogLevel(LOG_NONE);

    const int screenWidth = 1024;
    const int screenHeight = 768;

    InitWindow(screenWidth, screenHeight, "My Fake Chordie");
    SetTargetFPS(60);

    if (FileExists("fonts/cg.ttf")) {
        font = LoadFontEx("fonts/cg.ttf", 100, 0, 0);
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    } else {
        font = GetFontDefault();
    }

    ScanMidiDevicesToBuffer();

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_R)) {
            ScanMidiDevicesToBuffer();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        PrintToScreen(infoBuffer);
        DrawText("Press 'R' to Rescan", 50, 700, 20, GRAY);

        EndDrawing();
    }

    if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
    CloseWindow();
    return 0;
}