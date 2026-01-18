#if defined(_WIN32)
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

typedef struct tagMSG *LPMSG;

#include <windows.h>

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
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

#define true 1
#define false 0
#define START_NOTE 21
#define END_NOTE 108
#define TOTAL_WHITE_KEYS 52
#define TOTAL_KEYS 14

#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

// Standard Tone Pitch Class values for note names
// C-D-E-F-G-A-B with sharps and flats
// The value are for positioning as {sharp, flat} 
//      used the circle of fifths for positioning
// Example: C       = 0 sharps, 0 flats 
//          C# / Db = 7 sharps, 5 flats,
//      so that, moving clockwise adds sharps,
//      moving counter-clockwise adds flats, and so on.
// G is on the right of C with 1 sharp, 
//      so its position is always 1 step clockwise from C,
//          from that we notate it as {1, 1} (its position is always 1)

// The circle of fifths positions for all 12 notes
// is mapped as follows: ... - Bb - F - C - G - D - ...
const int8_t TPC_VALUES[12][2] = {
    {0, 12},    // 0:  C / B#
    {7, -5},    // 1:  C#/ Db
    {2, 2},     // 2:  D
    {9, -3},    // 3:  D#/ Eb
    {4, -8},    // 4:  E / Fb
    {-1, 11},   // 5:  F / E#
    {6, -6},    // 6:  F#/ Gb
    {1, 1},     // 7:  G
    {8, -4},    // 8:  G#/ Ab
    {3, 3},     // 9:  A
    {10, -2},   // 10: A#/ Bb
    {5, -7}     // 11: B / Cb
};

// ===================
// Global Variables
// -------------------
float globalGravityCenter = 1.0f;
bool use_auto_key = true;
uint8_t manual_key_center = 0;
volatile int keyState[128] = {0};
HMIDIIN hMidiDevice = NULL;
HMIDIOUT hMidiOut = NULL;

int currentDeviceId = 0;
int currentOutputDeviceId = 0;
// -------------------
// ===================

const char* val_to_tpc(int);
const char* get_note_name(int);
const char* estimate_key(float);
void update_gravity_center(int*, int, float);
void print_to_screen(const char*, int, Font, Vector2, int, Color);
void CALLBACK midi_proc(HMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);
void midi_out_open(int);
bool IsBlackKey(int);
const char* get_note_by_key(int);
bool contains_note(int*, int, int);
const char* AnalyzeChord(int*, int, int);
void midi_in_open(int);
void DrawKeyboard(float, float);

int main(void) {
    int windowWidth = 1200;
    int windowHeight = 600;
    int fps = 60;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(windowWidth, windowHeight, "Kunnic's Piano Engine");
    SetTargetFPS(fps);

    Image icon = LoadImage("chordie.ico"); 
    SetWindowIcon(icon);
    UnloadImage(icon);

    midi_in_open(currentDeviceId);
    midi_out_open(currentOutputDeviceId);

    Font font = GetFontDefault();
    if (FileExists("fonts/cg.ttf")) {
        font = LoadFontEx("fonts/cg.ttf", 200, 0, 0); 
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_DOWN)) {
            currentDeviceId--;
            if (currentDeviceId < 0) currentDeviceId = (int)midiInGetNumDevs() - 1;
            if (currentDeviceId < 0) currentDeviceId = 0;
            midi_in_open(currentDeviceId);
        }
        if (IsKeyPressed(KEY_UP)) {
            currentDeviceId++;
            if ((unsigned int)currentDeviceId >= midiInGetNumDevs()) currentDeviceId = 0;
            midi_in_open(currentDeviceId);
        }

        if (IsKeyPressed(KEY_PAGE_DOWN)) {
            int numOut = midiOutGetNumDevs();
            if (numOut > 0) {
                currentOutputDeviceId--;
                if (currentOutputDeviceId < 0) currentOutputDeviceId = numOut - 1;
                midi_out_open(currentOutputDeviceId);
            }
        }
        if (IsKeyPressed(KEY_PAGE_UP)) {
            int numOut = midiOutGetNumDevs();
            if (numOut > 0) {
                currentOutputDeviceId++;
                if (currentOutputDeviceId >= numOut) currentOutputDeviceId = 0;
                midi_out_open(currentOutputDeviceId);
            }
        }

        if (IsKeyPressed(KEY_TAB)) {
            use_auto_key = !use_auto_key;
            if (!use_auto_key) manual_key_center = (int)roundf(globalGravityCenter);
        }

        if (!use_auto_key) {
            if (IsKeyPressed(KEY_RIGHT)) manual_key_center++;
            if (IsKeyPressed(KEY_LEFT)) manual_key_center--;
            if (manual_key_center > 7) manual_key_center = 7;
            if (manual_key_center < -7) manual_key_center = -7;
            globalGravityCenter = (float)manual_key_center;
        }

        int activeNotes[128];
        int activeCount = 0;
        char noteStr[512] = "";
        char chordToDraw[64] = "";

        for (int midi_note = START_NOTE; midi_note <= END_NOTE; midi_note++) {
            if (keyState[midi_note] > 0) {
                int norm = midi_note % 12;
                bool exists = false;
                for (int j = 0; j < activeCount; j++) {
                    if (activeNotes[j] == norm) { exists = true; break; }
                }
                if (!exists) activeNotes[activeCount++] = norm;
            }
        }

        if (use_auto_key) {
            float currentRate = (activeCount >= 3) ? 0.05f : 0.005f;
            update_gravity_center(activeNotes, activeCount, currentRate);
        }

        for (int midi_note = START_NOTE; midi_note <= END_NOTE; midi_note++) {
            if (keyState[midi_note] > 0) {
                if (strlen(noteStr) > 0) strcat(noteStr, " ");
                strcat(noteStr, get_note_name(midi_note));
            }
        }

        if (activeCount > 0) {
            int lowestNote = -1;
            for (int i = START_NOTE; i <= END_NOTE; i++) {
                if (keyState[i] > 0) { lowestNote = i; break; }
            }
            const char* result = AnalyzeChord(activeNotes, activeCount, lowestNote);
            if (result) strcpy(chordToDraw, result);
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        float marginX = windowWidth * 0.02f;
        float marginY = windowHeight * 0.05f;

        print_to_screen("--- INPUT (Up/Down) ---", 20, font, (Vector2){windowWidth - marginX, marginY}, 1, GRAY);
        print_to_screen(TextFormat("ID: %d", currentDeviceId), 20, font, (Vector2){windowWidth - marginX, marginY + 20}, 1, LIGHTGRAY);
        
        MIDIINCAPS inCaps;
        if (midiInGetDevCaps(currentDeviceId, &inCaps, sizeof(MIDIINCAPS)) == MMSYSERR_NOERROR) {
            print_to_screen(inCaps.szPname, 20, font, (Vector2){windowWidth - marginX, marginY + 40}, 1, GREEN);
        } else {
            print_to_screen("No Input", 20, font, (Vector2){windowWidth - marginX, marginY + 40}, 1, RED);
        }

        float outputY = marginY + 70;
        print_to_screen("--- OUTPUT (PgUp/PgDn) ---", 20, font, (Vector2){windowWidth - marginX, outputY}, 1, GRAY);
        print_to_screen(TextFormat("ID: %d", currentOutputDeviceId), 20, font, (Vector2){windowWidth - marginX, outputY + 20}, 1, LIGHTGRAY);

        MIDIOUTCAPS outCaps;
        if (midiOutGetDevCaps(currentOutputDeviceId, &outCaps, sizeof(MIDIOUTCAPS)) == MMSYSERR_NOERROR) {
            print_to_screen(outCaps.szPname, 20, font, (Vector2){windowWidth - marginX, outputY + 40}, 1, YELLOW);
        } else {
            print_to_screen("No Output", 20, font, (Vector2){windowWidth - marginX, outputY + 40}, 1, RED);
        }

        float modeY = outputY + 70;
        if (use_auto_key) {
            print_to_screen("MODE: AUTO", 25, font, (Vector2){windowWidth - marginX, modeY}, 1, GREEN);
            print_to_screen("(Press TAB for Manual)", 20, font, (Vector2){windowWidth - marginX, modeY + 25}, 1, LIGHTGRAY);
        } else {
            print_to_screen("MODE: MANUAL", 25, font, (Vector2){windowWidth - marginX, modeY}, 1, ORANGE);
            print_to_screen("(Arrows L/R: Key)", 20, font, (Vector2){windowWidth - marginX, modeY + 25}, 1, LIGHTGRAY);
        }

        print_to_screen(TextFormat("Gravity: %.2f", globalGravityCenter), 30, font, (Vector2){marginX, marginY}, 0, LIGHTGRAY);
        print_to_screen(estimate_key(globalGravityCenter), 50, font, (Vector2){marginX, marginY + 40}, 0, SKYBLUE);

        DrawKeyboard((float)windowWidth, (float)windowHeight);

        if (strlen(chordToDraw) > 0) {
            print_to_screen(chordToDraw, 150, font, (Vector2){windowWidth / 2.0f, windowHeight / 5.0f}, 2, GOLD);
        }

        if (strlen(noteStr) > 0) {
            print_to_screen(noteStr, 30, font, (Vector2){marginX, windowHeight - 180}, 0, YELLOW);
        }

        EndDrawing();
    }

    if (hMidiDevice) { midiInStop(hMidiDevice); midiInClose(hMidiDevice); }
    if (hMidiOut) { midiOutClose(hMidiOut); }
    UnloadFont(font);
    CloseWindow();
    return 0;
}


// Converting int notation to TPC standard note name.
// Input: int number from -12 to 12
// Output: corresponding TPC note name
const char* val_to_tpc(int tpc)
{
    switch (tpc) {
        // NEG numbers are flats
        case -8:  return "Fb";
        case -7:  return "Cb";
        case -6:  return "Gb";
        case -5:  return "Db";
        case -4:  return "Ab";
        case -3:  return "Eb";
        case -2:  return "Bb";

        // Narutal TPC numbers
        case -1:  return "F";
        case 0:   return "C";
        case 1:   return "G";
        case 2:   return "D";
        case 3:   return "A";
        case 4:   return "E";
        case 5:   return "B";

        // POS numbers are sharps
        case 6:   return "F#";
        case 7:   return "C#";
        case 8:   return "G#";
        case 9:   return "D#";
        case 10:  return "A#";
        case 11:  return "E#";
        case 12:  return "B#";
        
        default:  return "##/bb";
    }
}

// Get note name from MIDI input number
// Input: MIDI note number (int)
// Output: note name in TPC standard with octave number
const char* get_note_name(int midi_input)
{
    int sharp_var, flat_var;
    int pitch_class;
    int octave;
    int final_tpc;
    float dist_sharp, dist_flat;
    static char buffer[8];
    
    pitch_class = midi_input % 12;
    octave = (midi_input / 12) - 1;

    // [0] = sharps (first column), [1] = flats (second column)
    sharp_var = TPC_VALUES[pitch_class][0];
    flat_var = TPC_VALUES[pitch_class][1];

    dist_sharp = fabsf((float)sharp_var - globalGravityCenter);
    dist_flat = fabsf((float)flat_var - globalGravityCenter);

    if (dist_sharp <= dist_flat) {
        final_tpc = sharp_var;
    } else {
        final_tpc = flat_var;
    }
    
    const char *tpc_name = val_to_tpc(final_tpc);
    sprintf(buffer, "%s%d", tpc_name, octave);

    return buffer;
}

const char* estimate_key(float gravity_center)
{
    int key_index = (int)roundf(gravity_center - 2.0f);
    switch (key_index) {
        case -6: return "Gb Major / Eb Minor";
        case -5: return "Db Major / Bb Minor";
        case -4: return "Ab Major / F Minor";
        case -3: return "Eb Major / C Minor";
        case -2: return "Bb Major / G Minor";
        case -1: return "F Major / D Minor";
        case 0:  return "C Major / A Minor";
        case 1:  return "G Major / E Minor";
        case 2:  return "D Major / B Minor";
        case 3:  return "A Major / F# Minor";
        case 4:  return "E Major / C# Minor";
        case 5:  return "B Major / G# Minor";
        case 6:  return "F# Major / D# Minor";
        case 7:  return "C# Major / A# Minor";
        default: 
            if (key_index > 7) return "Sharp Side (C#+)";
            if (key_index < -6) return "Flat Side (Gb-)";
            return "Unknown";
    }
}

void update_gravity_center(int *active_notes, int count, float rate)
{
    int sum_gravity;
    int sharp_val, flat_val;
    float sharp_dist, flat_dist;
    int note_class;

    if (count == 0) {
        return;
    }

    sum_gravity = 0;

    for(int i=0; i<count; i++) {
        note_class = active_notes[i] % 12;
        
        sharp_val = TPC_VALUES[note_class][0];
        flat_val = TPC_VALUES[note_class][1];

        sharp_dist = fabsf((float)sharp_val - globalGravityCenter);
        flat_dist = fabsf((float)flat_val - globalGravityCenter);
        if (sharp_dist <= flat_dist) {
            sum_gravity += sharp_val;
        } else {
            sum_gravity += flat_val;
        }
    }

    float target = (float)sum_gravity / count;
    globalGravityCenter = (globalGravityCenter * (1.0f - rate)) + (target * rate);
}

void CALLBACK midi_proc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    unsigned char status, note, vel;

    if (wMsg == MIM_DATA) {
        // dwParam1 contains the MIDI message
        // 32 bits, 8 bits each: [status][note][velocity]
        // Example: 0x00[ss][nn][vv]
        // Shift bit >> and mask to extract each byte
        status = dwParam1 & 0xFF;
        note = (dwParam1 >> 8) & 0xFF;
        vel = (dwParam1 >> 16) & 0xFF;

        if ((status & 0xF0) == 0x90 && vel > 0) {
            keyState[note] = vel;
        } else if (((status & 0xF0) == 0x80) || ((status & 0xF0) == 0x90 && vel == 0)) {
            keyState[note] = 0;
        }

        if (hMidiOut) {
            midiOutShortMsg(hMidiOut, (DWORD)dwParam1);
        }
    }
}

void print_to_screen(const char* text, int size, Font font, Vector2 position, int align, Color color) {
    // align: 0 = left, 1 = right, 2 = center
    float spacing = 2.0f;
    Vector2 adjustedPos = position;
    
    if (align == 1 || align == 2) {
        Vector2 textSize = MeasureTextEx(font, text, (float)size, spacing);
        if (align == 1) {
            adjustedPos.x = position.x - textSize.x;
        } else if (align == 2) {
            adjustedPos.x = position.x - textSize.x / 2.0f;
        }
    }
    
    DrawTextEx(font, text, adjustedPos, (float)size, spacing, color);
}

bool IsBlackKey(int midiNote) {
    int n = midiNote % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

const char* get_note_by_key(int pitch_class) {
    int sharp_var = TPC_VALUES[pitch_class][0];
    int flat_var = TPC_VALUES[pitch_class][1];

    float dist_sharp = fabsf((float)sharp_var - globalGravityCenter);
    float dist_flat = fabsf((float)flat_var - globalGravityCenter);

    if (dist_sharp <= dist_flat) {
        return val_to_tpc(sharp_var);
    } else {
        return val_to_tpc(flat_var);
    }
}

bool contains_note(int *notes, int count, int targetNorm) {
    for (int i = 0; i < count; i++) {
        if (notes[i] == targetNorm) return true;
    }
    return false;
}

const char *AnalyzeChord(int *notes, int count, int lowestNoteMIDI)
{
    static char buffer[64];
    buffer[0] = '\0';

    if (count < 2) return "";

    for (int i = 0; i < count; i++) {
        int root = notes[i];

        bool m2 = contains_note(notes, count, (root + 1) % 12);
        bool M2 = contains_note(notes, count, (root + 2) % 12);
        bool m3 = contains_note(notes, count, (root + 3) % 12);
        bool M3 = contains_note(notes, count, (root + 4) % 12);
        bool P4 = contains_note(notes, count, (root + 5) % 12);
        bool d5 = contains_note(notes, count, (root + 6) % 12);
        bool P5 = contains_note(notes, count, (root + 7) % 12);
        bool m6 = contains_note(notes, count, (root + 8) % 12);
        bool M6 = contains_note(notes, count, (root + 9) % 12);
        bool m7 = contains_note(notes, count, (root + 10) % 12);
        bool M7 = contains_note(notes, count, (root + 11) % 12);

        char quality[32] = "";
        char extension[32] = "";
        bool match = false;

        if (M3 && m7) {
            strcpy(quality, "");
            if (M2) strcpy(extension, "9");
            else if (m2) strcpy(extension, "7b9");
            else if (m3) strcpy(extension, "7#9");
            else strcpy(extension, "7");
            match = true;
        }
        else if (M3 && M7) {
            strcpy(quality, "maj");
            if (M2) strcpy(extension, "9");
            else strcpy(extension, "7");
            match = true;
        }
        else if (m3 && m7) {
            strcpy(quality, "m");
            if (M2) strcpy(extension, "9");
            else if (m6) strcpy(extension, "7b5");
            else strcpy(extension, "7");
            match = true;
        }
        else if (m3 && d5 && M6) {
            strcpy(quality, "dim7");
            match = true;
        }
        else if (!m7 && !M7) {
            if (M3) {
                if (M2) {
                    strcpy(quality, "add9");
                    match = true;
                }
                else if (M6) {
                    strcpy(quality, "6");
                    match = true;
                }
                else {
                    strcpy(quality, "");
                    match = true;
                }
            }
            else if (m3) {
                if (M2) {
                    strcpy(quality, "m(add9)");
                    match = true;
                }
                else if (M6) {
                    strcpy(quality, "m6");
                    match = true;
                }
                else {
                    strcpy(quality, "m");
                    match = true;
                }
            }
            else if (P4) {
                strcpy(quality, "sus4");
                match = true;
            }
            else if (M2 && !m3 && !M3) {
                strcpy(quality, "sus2");
                match = true;
            }
            else if (m3 && d5) {
                strcpy(quality, "dim");
                match = true;
            }
            else if (M3 && m6) {
                strcpy(quality, "aug");
                match = true;
            }
        }

        if (!match && count == 2 && P5) {
            strcpy(quality, "5");
            match = true;
        }

        if (match) {
            const char* rootName = get_note_by_key(root);
            sprintf(buffer, "%s%s%s", rootName, quality, extension);

            int bassNoteVal = lowestNoteMIDI % 12;
            if (bassNoteVal != root) {
                const char* bassName = get_note_by_key(bassNoteVal);
                char bassStr[16];
                sprintf(bassStr, "/%s", bassName);
                strcat(buffer, bassStr);
            }
            return buffer;
        }
    }

    return "";
}

void midi_in_open(int deviceId)
{
    if (hMidiDevice) {
        midiInStop(hMidiDevice);
        midiInClose(hMidiDevice);
        hMidiDevice = NULL;
    }
    memset((void *)keyState, 0, sizeof(keyState));

    if ((unsigned int)deviceId < midiInGetNumDevs()) {
        midiInOpen(&hMidiDevice, deviceId, (DWORD_PTR)(void *)midi_proc, 0, CALLBACK_FUNCTION);
        midiInStart(hMidiDevice);
        currentDeviceId = deviceId;
    }
}

void DrawKeyboard(float screenW, float screenH)
{
    float whiteKeyWidth = screenW / TOTAL_WHITE_KEYS;
    float whiteKeyHeight = 100.0f;
    float blackKeyWidth = whiteKeyWidth * 0.65f;
    float blackKeyHeight = whiteKeyHeight * 0.6f;
    float keyboardY = screenH - whiteKeyHeight;

    int whiteIndex = 0;
    for (int i = START_NOTE; i <= END_NOTE; i++) {
        if (!IsBlackKey(i)) {
            float x = whiteIndex * whiteKeyWidth;
            bool isActive = (keyState[i] > 0);
            DrawRectangleRec((Rectangle){x, keyboardY, whiteKeyWidth - 2, whiteKeyHeight}, isActive ? GRAY : RAYWHITE);
            whiteIndex++;
        }
    }

    whiteIndex = 0;
    for (int i = START_NOTE; i <= END_NOTE; i++) {
        if (!IsBlackKey(i)) {
            whiteIndex++;
        } else {
            float x = (whiteIndex * whiteKeyWidth) - (blackKeyWidth / 2.0f);
            bool isActive = (keyState[i] > 0);
            DrawRectangleRec((Rectangle){x, keyboardY, blackKeyWidth, blackKeyHeight}, isActive ? RED : BLACK);
        }
    }
}

void midi_out_open(int deviceId)
{
    if (hMidiOut) {
        midiOutClose(hMidiOut);
        hMidiOut = NULL;
    }

    unsigned int numDevs = midiOutGetNumDevs();
    if (numDevs == 0) return;

    if (deviceId < 0) deviceId = numDevs - 1;
    if ((unsigned int)deviceId >= numDevs) deviceId = 0;

    MMRESULT result = midiOutOpen(&hMidiOut, deviceId, 0, 0, CALLBACK_NULL);
    if (result == MMSYSERR_NOERROR) {
        currentOutputDeviceId = deviceId;
    }
}