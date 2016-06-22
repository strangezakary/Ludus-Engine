/*
    Creation Date: 26-03-2016
    Creator: Zakary Strange
*/

#if !defined(LUDUS_PLATFORM_H)

#include <stddef.h>
#include <stdint.h>

#define WINDOW_NAME "Ludus Test Window"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define internal static
#define global_variable static
#define local_persist static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef uintptr_t memory_index;

typedef int8 s8;
typedef int8 s08;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint8 u08;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef real32 r32;
typedef real64 r64;

#ifdef LUDUS_SLOW
#define Assert(Expression)                                                                                             \
    if(!(Expression))                                                                                                  \
    {                                                                                                                  \
        *(int*)0 = 0;                                                                                                  \
    }
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

internal inline u32
StringLength(char* String)
{
    u32 Count = 0;
    while(*String++)
    {
        ++Count;
    }

    return (Count);
}

internal void
CatStrings(memory_index SourceACount,
           char* SourceA,
           memory_index SourceBCount,
           char* SourceB,
           memory_index DestCount,
           char* Dest)
{
    for(memory_index Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(memory_index Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal b32
StringsAreEqual(char* A, char* B)
{
    while(*A && *B && *A == *B)
    {
        ++A;
        ++B;
    }

    b32 Result = (*A == 0 && *B == 0);
    return (Result);
}

// NOTE(zak): If this is slow bother kiljacken
internal void
CopyMemory(uint8* Dst, uint8* Src, memory_index Length)
{
    for(memory_index Index = 0; Index < Length; Index++)
    {
        *(Dst + Index) = *(Src + Index);
    }
}

internal void
Zero(void* Memory, memory_index Size)
{
    unsigned char* MemoryResult = (unsigned char*)Memory;
    while(Size > 0)
    {
        *MemoryResult = 0;
        MemoryResult++;
        Size--;
    }
}

internal void
ReAlloc()
{
}

inline u32
SafeTruncateUInt64(u64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (uint32)Value;
    return (Result);
}

typedef struct read_file_result
{
    u32 ContentsSize;
    void* Contents;
} read_file_result;

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void* Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char* FileName)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char* FileName, u32 MemorySize, void* Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

#define PLATFORM_LOG(name) void name(char* String)
typedef PLATFORM_LOG(platform_log);

#define PLATFORM_OUTPUT_DEBUG_STRING(name) void name(char* String)
typedef PLATFORM_OUTPUT_DEBUG_STRING(platform_output_debug_string);

#define PLATFORM_GET_FILE_TIME(name) s64 name(char* FilePath)
typedef PLATFORM_GET_FILE_TIME(platform_get_file_time);

typedef struct platform_api
{
    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;
    platform_free_file_memory* FreeFileMemory;
    platform_read_entire_file* ReadEntireFile;
    platform_write_entire_file* WriteEntireFile;
    platform_log* Log;
    platform_output_debug_string* OutputDebugString;
    platform_get_file_time* GetFileTime;
} platform_api;

typedef struct game_button_state
{
    int HalfTransitionCount;
    b32 EndedDown;
} game_button_state;

typedef struct game_controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    r32 LeftStickAverageX;
    r32 LeftStickAverageY;
    r32 RightStickAverageX;
    r32 RightStickAverageY;

    union
    {
        game_button_state Buttons[15];
        struct
        {
            game_button_state MoveForward;
            game_button_state MoveBackward;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state DPadUp;
            game_button_state DPadDown;
            game_button_state DPadLeft;
            game_button_state DPadRight;

            game_button_state ButtonDown;  // A on Xbox
            game_button_state ButtonRight; // B on Xbox
            game_button_state ButtonLeft;  // X on Xbox
            game_button_state ButtonUp;    // Y on Xbox

            game_button_state Start;
            game_button_state Select; // Options on PS4 | PS3

            // NOTE(zak): All buttons must be added above this line

            game_button_state Terminator;
        };
    };

} game_controller_input;

enum game_input_mouse_button
{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Extended0,
    PlatformMouseButton_Extended1,

    PlatformMouseButton_Count,
};

typedef struct game_input
{
    game_controller_input Controllers[5];

    game_button_state MouseButtons[PlatformMouseButton_Count];
    r32 MouseX, MouseY;
    b32 ShiftDown, AltDown, ControlDown;

    r32 dtForFrame;
} game_input;

inline game_controller_input*
GetController(game_input* Input, u32 ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));

    game_controller_input* Result = &Input->Controllers[ControllerIndex];
    return (Result);
}

inline b32
WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) || ((State.HalfTransitionCount == 1) && (State.EndedDown)));

    return (Result);
}

typedef struct game_memory
{
    s64 PermanentStorageSize;
    void* PermanentStorage;

    s64 TransientStorageSize;
    void* TransientStorage;

    platform_api PlatformAPI;
    b32 ExecuteableReloaded;
    b32 DebugMode;

} game_memory;

typedef struct audio_sample_request
{
    s32 SampleRate;
    s32 ChannelCount;
    s32 SampleCount;

    float* SampleBuffer;
} audio_sample_request;

global_variable platform_api Platform;

#define GAME_UPDATE_AND_RENDER(name)                                                                                   \
    void name(game_memory* Memory, game_input* Input, struct render_buffer* RenderBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GENERATE_AUDIO_SAMPLES(name) void name(game_memory* Memory, audio_sample_request* SampleRequest)
typedef GAME_GENERATE_AUDIO_SAMPLES(game_generate_audio_samples);

#ifndef __cplusplus
#define true 1
#define false 0
#endif

#define LUDUS_PLATFORM_H
#endif
