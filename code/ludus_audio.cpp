/*
    Creation Date: 30-04-2016
    Creator: Zakary Strange
*/

#include "ludus_audio.h"

s32
SampleSize(sound Sound)
{
    s32 Result = 0;
    switch(Sound.SampleType)
    {
        case Signed16:
        {
            Result = 2;
        }
        break;
        case Signed24:
        {
            Result = 3;
        }
        break;
        case Float32:
        {
            Result = 4;
        }
        break;
        default:
        {
        }
    }
    Result *= (s32)Sound.ChannelCount;
    return Result;
}

internal sound
LoadSoundFromWavData(memory_arena* Arena, void* WavData, u64 WavDataSize)
{
    sound Result          = {};
    wav_header* WavHeader = (wav_header*)WavData;
    b32 Success           = false;
    if(WavDataSize > sizeof(wav_header))
    {
        b32 RIFFTest = WavHeader->RIFF[0] == 'R' && WavHeader->RIFF[1] == 'I' && WavHeader->RIFF[2] == 'F' &&
                       WavHeader->RIFF[3] == 'F';
        b32 WAVETest = WavHeader->WAVE[0] == 'W' && WavHeader->WAVE[1] == 'A' && WavHeader->WAVE[2] == 'V' &&
                       WavHeader->WAVE[3] == 'E';
        if(RIFFTest && WAVETest)
        {
            u64 ChunkOffset = sizeof(wav_header);
            b32 FormatFound = false;
            b32 DataFound   = false;
            u32 DataSize    = 0;
            u8* DataPtr     = 0;
            while(ChunkOffset + sizeof(wav_chunk_header) < WavDataSize)
            {
                wav_chunk_header* ChunkWalker = (wav_chunk_header*)((u8*)WavData + ChunkOffset);
                b32 fmt32Test                 = ChunkWalker->Name[0] == 'f' && ChunkWalker->Name[1] == 'm' &&
                                ChunkWalker->Name[2] == 't' && ChunkWalker->Name[3] == ' ';
                b32 dataTest = ChunkWalker->Name[0] == 'd' && ChunkWalker->Name[1] == 'a' &&
                               ChunkWalker->Name[2] == 't' && ChunkWalker->Name[3] == 'a';
                if(fmt32Test)
                {
                    wav_format_chunk* FormatChunk = (wav_format_chunk*)ChunkWalker;
                    if(FormatChunk->Format == 1 && FormatChunk->BitsPerSample == 16)
                    {
                        Result.ChannelCount = FormatChunk->ChannelCount;
                        Result.SampleRate   = FormatChunk->SampleRate;
                        Result.SampleType   = Signed16;
                        FormatFound         = true;
                    }
                    else if(FormatChunk->Format == 1 && FormatChunk->BitsPerSample == 24)
                    {
                        Result.ChannelCount = FormatChunk->ChannelCount;
                        Result.SampleRate   = FormatChunk->SampleRate;
                        Result.SampleType   = Signed24;
                        FormatFound         = true;
                    }
                    else if(FormatChunk->Format == 3 && FormatChunk->BitsPerSample == 32)
                    {
                        Result.ChannelCount = FormatChunk->ChannelCount;
                        Result.SampleRate   = FormatChunk->SampleRate;
                        Result.SampleType   = Float32;
                        FormatFound         = true;
                    }
                }
                else if(dataTest)
                {
                    wav_data_chunk* DataChunk = (wav_data_chunk*)ChunkWalker;
                    DataFound                 = true;
                    DataSize                  = DataChunk->Size;
                    DataPtr                   = ((u8*)DataChunk + sizeof(wav_data_chunk));
                }
                ChunkOffset += ChunkWalker->Size + sizeof(wav_chunk_header);
            }

            if(FormatFound && DataFound)
            {
                Assert(SampleSize(Result));
                Result.SampleCount = DataSize / SampleSize(Result);
                if((u64)(DataPtr - (u8*)WavData) + DataSize <= WavDataSize)
                {
                    memory_index SampleAllocSize =
                        (memory_index)(Result.ChannelCount * sizeof(float) * Result.SampleCount);
                    Result.Samples = (float*)PushSize(Arena, SampleAllocSize);
                    switch(Result.SampleType)
                    {
                        case Signed16:
                        {
                            float SampleDivisor = (float)((1 << (16 - 1)) - 1);
                            s16* SourceSample   = (s16*)DataPtr;
                            if(Result.ChannelCount == 1)
                            {
                                for(s64 II = 0; II < Result.SampleCount; ++II)
                                {
                                    Result.Samples[II] = SourceSample[II] / SampleDivisor;
                                }
                            }
                            else
                            {
                                for(s64 II = 0; II < Result.SampleCount; ++II)
                                {
                                    Result.Samples[2 * II + 0] = SourceSample[2 * II + 0] / SampleDivisor;
                                    Result.Samples[2 * II + 1] = SourceSample[2 * II + 1] / SampleDivisor;
                                }
                            }
                        }
                        break;
                        case Signed24:
                        {
                            float SampleDivisor = (float)((1 << (24 - 1)) - 1);
                            u32 BitMask         = (1 << 24) - 1;
                            if(Result.ChannelCount == 1)
                            {
                                for(s64 II = 0; II < Result.SampleCount; ++II)
                                {
                                    u32 Sample         = (*(u32*)(DataPtr + 3 * (II)) & BitMask) << 8;
                                    s32 SampleVal      = ((s32)Sample) >> 8;
                                    Result.Samples[II] = SampleVal / SampleDivisor;
                                }
                            }
                            else
                            {
                                for(s64 II = 0; II < Result.SampleCount; ++II)
                                {
                                    u32 Left                   = (*(u32*)(DataPtr + 3 * (2 * II + 0)) & BitMask) << 8;
                                    s32 LeftVal                = ((s32)Left) >> 8;
                                    u32 Right                  = (*(u32*)(DataPtr + 3 * (2 * II + 1)) & BitMask) << 8;
                                    s32 RightVal               = ((s32)Right) >> 8;
                                    Result.Samples[2 * II + 0] = LeftVal / SampleDivisor;
                                    Result.Samples[2 * II + 1] = RightVal / SampleDivisor;
                                }
                            }
                        }
                        break;
                        case Float32:
                        {
                            CopyMemory((u8*)Result.Samples, DataPtr, SampleAllocSize);
                        }
                        break;
                    }
                    Success = true;
                }
            }
        }
    }
    if(Success)
    {
        return Result;
    }
    else
    {
        Zero((void*)&Result, sizeof(Result));
        return Result;
    }
}

internal sound
LoadSoundFromWavFile(memory_arena* Arena, char* File)
{
    read_file_result WavFile = Platform.ReadEntireFile(File);
    sound result             = {};
    if(WavFile.Contents)
    {
        result = LoadSoundFromWavData(Arena, WavFile.Contents, WavFile.ContentsSize);
        Platform.FreeFileMemory(WavFile.Contents);
    }
    return result;
}

internal sound_play_state_id
PlaySound(audio_state* AudioState, sound* Sound)
{
    sound_play_state_id Result;
    if(AudioState->FreeSoundCount)
    {
        Result = AudioState->FreeSoundIDs[--AudioState->FreeSoundCount];
    }
    else
    {
        Result = (sound_play_state_id)++AudioState->AllocatedSoundCount;
    }
    sound_play_state* PlayState   = AudioState->SoundPlayStates + (Result - 1);
    PlayState->Samples            = Sound->Samples;
    PlayState->SampleCount        = Sound->SampleCount;
    PlayState->ChannelCount       = Sound->ChannelCount;
    PlayState->SampleRate         = Sound->SampleRate;
    PlayState->Volume             = 1.0f;
    PlayState->Speed              = 1.0f;
    PlayState->Next               = AudioState->FirstPlayingSound;
    AudioState->FirstPlayingSound = Result;
    return Result;
}

internal sound_play_state*
GetPlayState(audio_state* AudioState, sound_play_state_id ID)
{
    return AudioState->SoundPlayStates + (ID - 1);
}

internal void
PushSoundSamples(audio_state* AudioState, audio_sample_request* AudioSampleRequest)
{
    sound_play_state_id* CurID = &AudioState->FirstPlayingSound;
    s64 SubResolution          = 1000000;
    while(*CurID)
    {
        sound_play_state* PlayState = AudioState->SoundPlayStates + (*CurID - 1);
        if(!PlayState->Paused && (s64)(PlayState->Speed * SubResolution) != 0)
        {
            s64 SamplesToPlay = MIN((ABS(PlayState->SampleCount * SubResolution - PlayState->CurrentSubSample) /
                                     (s64)(PlayState->Speed * SubResolution)),
                                    (s64)AudioSampleRequest->SampleCount);
            if(PlayState->Loop)
            {
                SamplesToPlay = AudioSampleRequest->SampleCount;
            }
            float SampleRatio = float(PlayState->SampleRate) / AudioSampleRequest->SampleRate;
            float CurVolume   = AudioState->Volume * PlayState->Volume;
            s32 ChannelCount  = AudioSampleRequest->ChannelCount;
            for(s64 II = 0; II < SamplesToPlay; ++II)
            {
                // TODO(ronald): probably going to need a filter for resampling or to
                // lock to certain sample rate
                s64 SampleID = MOD(PlayState->CurrentSubSample / SubResolution, PlayState->SampleCount);
                if(AudioSampleRequest->ChannelCount >= 2)
                {
                    if(PlayState->ChannelCount == 2)
                    {
                        AudioSampleRequest->SampleBuffer[ChannelCount * II + 0] +=
                            CurVolume * PlayState->Samples[2 * SampleID + 0];
                        AudioSampleRequest->SampleBuffer[ChannelCount * II + 1] +=
                            CurVolume * PlayState->Samples[2 * SampleID + 1];
                    }
                    else if(PlayState->ChannelCount == 1)
                    {
                        AudioSampleRequest->SampleBuffer[ChannelCount * II + 0] +=
                            CurVolume * PlayState->Samples[SampleID];
                        AudioSampleRequest->SampleBuffer[ChannelCount * II + 1] +=
                            CurVolume * PlayState->Samples[SampleID];
                    }
                }
                else
                {
                    if(PlayState->ChannelCount == 2)
                    {
                        // TODO(ronald): figure out if this is the expected mix down
                        AudioSampleRequest->SampleBuffer[II] +=
                            CurVolume * (PlayState->Samples[2 * SampleID + 0] + PlayState->Samples[2 * SampleID + 1]) /
                            2;
                    }
                    else if(PlayState->ChannelCount == 1)
                    {
                        AudioSampleRequest->SampleBuffer[II] += CurVolume * PlayState->Samples[SampleID];
                    }
                }
                PlayState->CurrentSubSample += (s64)(SampleRatio * PlayState->Speed * SubResolution);
            }
        }

        if(PlayState->Loop)
        {
            PlayState->CurrentSubSample = MOD(PlayState->CurrentSubSample, (SubResolution * PlayState->SampleCount));
        }

        if(PlayState->CurrentSubSample >= PlayState->SampleCount * SubResolution)
        {
            AudioState->FreeSoundIDs[AudioState->FreeSoundCount++] = *CurID;
            *CurID                                                 = PlayState->Next;
        }
        else
        {
            CurID = &PlayState->Next;
        }
    }
}
