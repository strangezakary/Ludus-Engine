/*
    Creation Date: 01-04-2016
    Creator: Ronald Fenelus
*/

#if !defined(LUDUS_AUDIO_H)

#include "ludus_platform.h"
#include "ludus_shared.h"

enum sound_sample_type
{
    Signed16,
    Signed24,
    Float32,
};

typedef struct sound
{
    float* Samples;
    s64 SampleRate;
    s64 SampleCount;
    s64 ChannelCount;
    sound_sample_type SampleType;
} sound;

typedef u16 sound_play_state_id;

typedef struct sound_play_state
{
    float* Samples;
    s64 CurrentSubSample;
    s64 ChannelCount;
    s64 SampleCount;
    s64 SampleRate;
    float Volume;
    float Speed;
    bool Loop;
    bool Paused;
    sound_play_state_id Next;
} sound_play_state;

#define MAX_PLAYING_SOUNDS 1024

typedef struct audio_state
{
    sound_play_state SoundPlayStates[MAX_PLAYING_SOUNDS];
    sound_play_state_id FreeSoundIDs[MAX_PLAYING_SOUNDS];
    u32 AllocatedSoundCount;
    u32 FreeSoundCount;
    float Volume;
    sound_play_state_id FirstPlayingSound;
} audio_state;

#pragma pack(push, 1)
typedef struct wav_chunk_header
{
    char Name[4];
    u32 Size;
} wav_chunk_header;

typedef struct wav_format_chunk
{
    char fmt32[4];
    u32 Size;
    u16 Format;
    u16 ChannelCount;
    u32 SampleRate;
    u32 AverageBytesPerSecond;
    u16 BlockAlign;
    u16 BitsPerSample;
} wav_format_chunk;

typedef struct wav_data_chunk
{
    char DATA[4];
    u32 Size;
} wav_data_chunk;

typedef struct wav_header
{
    char RIFF[4];
    u32 Size;
    char WAVE[4];
} wav_header;
#pragma pack(pop)

internal s32
SampleSize(sound Sound);
internal sound
LoadSoundFromWavData(memory_arena* Arena, void* WavData, u64 WavDataSize);
internal sound
LoadSoundFromWavFile(memory_arena* Arena, char* File);
internal sound_play_state_id
PlaySound(audio_state* AudioState, sound* Sound);
internal void
PushSoundSamples(audio_state* AudioState, audio_sample_request* AudioSampleRequest);

#define LUDUS_AUDIO_H
#endif
