/*
    Creation Date: 01-05-2016
    Creator: Florian Behr & Alex Baines
*/

#include <AL/al.h>
#include <AL/alc.h>

typedef struct
{
    b32 Initialized;
    b32 Playing;
    int SampleRate;
    int NumChannels;
    r32* AggregateBuffersData;
    int AggregateBuffersSize;
    int NumBuffers;
    int NextBufferToFill;
    int SamplesPerBuffer;
    int SamplesBufferSize;
    ALuint* ALBuffers;
    ALuint ALSource;
    ALCcontext* ALContext;
    ALCdevice* ALDevice;
    ALuint* BuffersToFill;
} linux_openal_audio_state;

global_variable linux_openal_audio_state ALState;

internal int
LinuxOpenALInit()
{
    GlobalAudioBackend           = linux_audio_backend_openal;
    ALenum ALerror               = 0;
    ALState.Playing              = 0;
    ALState.SampleRate           = 48000;
    ALState.NumChannels          = 2;
    ALState.NumBuffers           = 4;
    ALState.SamplesPerBuffer     = (int)((r32)ALState.SampleRate * (1.0f / 60.0f));
    ALState.SamplesBufferSize    = ALState.SamplesPerBuffer * ALState.NumChannels * sizeof(r32);
    ALState.NextBufferToFill     = 0;
    ALState.AggregateBuffersSize = ALState.SamplesBufferSize * ALState.NumBuffers;
    ALState.AggregateBuffersData = (r32*)malloc(ALState.AggregateBuffersSize);
    ALState.ALBuffers            = (ALuint*)malloc(ALState.NumBuffers * sizeof(ALuint));
    ALState.BuffersToFill        = (ALuint*)malloc(ALState.NumBuffers * sizeof(ALuint));

    ALState.ALDevice = alcOpenDevice(NULL); // select the "preferred device"
    if(ALState.ALDevice)
    {
        ALState.Initialized = true;
        ALState.ALContext   = alcCreateContext(ALState.ALDevice, NULL);
        alcMakeContextCurrent(ALState.ALContext);
        if(alIsExtensionPresent("AL_EXT_float32") && alcIsExtensionPresent(ALState.ALDevice, "ALC_EXT_disconnect"))
        {
            alGetError(); // clear error code

            alGenBuffers(ALState.NumBuffers, ALState.ALBuffers);
            if((ALerror = alGetError()) != AL_NO_ERROR)
            {
                LinuxLogF("AL init: alGenBuffers : %i", ALerror);
                ALState.Initialized = false;
                GlobalAudioBackend  = linux_audio_backend_none;
            }

            alGenSources(1, &ALState.ALSource);
            if((ALerror = alGetError()) != AL_NO_ERROR)
            {
                LinuxLogF("AL init: alGenSources 1 : %i", ALerror);
                ALState.Initialized = false;
                GlobalAudioBackend  = linux_audio_backend_none;
            }
        }
        else
        {
            alcMakeContextCurrent(0);
            alcDestroyContext(ALState.ALContext);
            alcCloseDevice(ALState.ALDevice);
            ALState.Initialized = false;
            GlobalAudioBackend  = linux_audio_backend_none;
            return 0;
        }
    }
    return 1;
}

internal void
LinuxOpenALShutdown()
{
    ALCcontext* Context = alcGetCurrentContext();
    ALCdevice* Device   = alcGetContextsDevice(Context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(Context);
    alcCloseDevice(Device);
}

internal void
LinuxOpenALUpdate(game_memory* GameMemory)
{
    int NumBuffersToFill;
    ALint SourceState     = 0;
    ALint DeviceConnected = 0;

    if(!ALState.Initialized)
    {
        return;
    }

    alcGetIntegerv(ALState.ALDevice, ALC_CONNECTED, sizeof(DeviceConnected), &DeviceConnected);
    if(!DeviceConnected)
    {
        LinuxOpenALShutdown();
        LinuxOpenALInit();
    }
    alGetSourcei(ALState.ALSource, AL_SOURCE_STATE, &SourceState);
    ALState.Playing = SourceState == AL_PLAYING;
    if(!ALState.Playing)
    {
        alSourceStop(ALState.ALSource);
        alGetSourcei(ALState.ALSource, AL_BUFFERS_PROCESSED, &NumBuffersToFill);
        alSourceUnqueueBuffers(ALState.ALSource, NumBuffersToFill, ALState.BuffersToFill);
        NumBuffersToFill = ALState.NumBuffers;
        memcpy(ALState.BuffersToFill, ALState.ALBuffers, NumBuffersToFill * sizeof(*ALState.ALBuffers));
    }
    else
    {
        alGetSourcei(ALState.ALSource, AL_BUFFERS_PROCESSED, &NumBuffersToFill);
        alSourceUnqueueBuffers(ALState.ALSource, NumBuffersToFill, ALState.BuffersToFill);
    }

    ALuint ALerror = 0;
    for(int i = 0; i < NumBuffersToFill; i++)
    {
        r32* BufferData =
            ALState.AggregateBuffersData + (ALState.SamplesPerBuffer * ALState.NumChannels) * ALState.NextBufferToFill;
        memset(BufferData, 0, ALState.SamplesBufferSize);
        audio_sample_request SampleRequest = {};
        SampleRequest.SampleRate           = ALState.SampleRate;
        SampleRequest.ChannelCount         = ALState.NumChannels;
        SampleRequest.SampleCount          = ALState.SamplesPerBuffer;
        SampleRequest.SampleBuffer         = BufferData;
        GameCode.GameGenerateAudioSamples(GameMemory, &SampleRequest);

        alBufferData(ALState.BuffersToFill[i],
                     AL_FORMAT_STEREO_FLOAT32,
                     BufferData,
                     ALState.SamplesBufferSize,
                     ALState.SampleRate);
        if((ALerror = alGetError()) != AL_NO_ERROR)
        {
            if(ALerror == AL_OUT_OF_MEMORY)
            {
                LinuxX11PlatformLog("AL alBufferData: Out of Memory");
            }
            else if(ALerror == ALC_INVALID_VALUE)
            {
                LinuxX11PlatformLog("AL alBufferData: Invalid Value");
            }
            else if(ALerror == ALC_INVALID_ENUM)
            {
                LinuxX11PlatformLog("AL alBufferData: Invalid Enum");
            }
            else if(ALerror == ALC_INVALID_DEVICE)
            {
                LinuxX11PlatformLog("AL alBufferData: Invalid Device");
            }
            else
            {
                LinuxLogF("AL alBufferData: Error code %u", ALerror);
            }
        }
        ALState.NextBufferToFill += 1;
        if(ALState.NextBufferToFill >= ALState.NumBuffers)
            ALState.NextBufferToFill = 0;
    }
    alSourceQueueBuffers(ALState.ALSource, NumBuffersToFill, ALState.BuffersToFill);
    if(!ALState.Playing)
    {
        alSourcePlay(ALState.ALSource);
        ALState.Playing = true;
        LinuxX11PlatformLog("AL: Not playing, starting");
    }
}
