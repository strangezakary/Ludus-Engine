/*
    Creation Date: 01-04-2016
    Creator: Andrew Spiering
*/

#ifndef OSX_LUDUS_H
#define OSX_LUDUS_H

#define StdOut 1
#define StdErr 2

typedef struct
{
    void* GameCodeDLL;
    time_t DLLastWriteTime;


    game_update_and_render* GameUpdateAndRender;
    b32 IsValid;
} osx_game_code;

typedef struct
{
    char EXEFileName[4096];
    char* OnePastLastEXEFileNameSlash;
} osx_state;

global_variable osx_game_code GameCode;
global_variable render_buffer RenderBuffer;
global_variable game_memory GameMemory;

global_variable b32 GlobalRunning;
global_variable b32 GLFallBack;
global_variable b32 GlobalPause;
global_variable int GlobalDataDirHandle;

#endif
