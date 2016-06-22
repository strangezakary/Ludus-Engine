/*
    Creation Date: 28-03-2016
    Creator: Zakary Strange
*/

#if !defined(WIN32_LUDUS_H)

typedef struct
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    game_update_and_render* GameUpdateAndRender;
    game_generate_audio_samples* GameGenerateAudioSamples;

    b32 IsValid;
} win32_game_code;

typedef struct
{
    uint64 TotalSize;
    void* GameMemoryBlock;

    char EXEFileName[4096];
    char* OnePastLastEXEFileNameSlash;
} win32_state;

#define WIN32_LUDUS_H
#endif
