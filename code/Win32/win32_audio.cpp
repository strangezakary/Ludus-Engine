/*
    Creation Date: 30-04-2016
    Creator: Zakary Strange
*/

#include <Audioclient.h>
#include <Mmdeviceapi.h>

typedef struct
{
    b32 Enabled;
    b32 DeviceLost;
    s32 SampleRate;
    s32 SampleSize;
    s32 ChannelCount;
    s32 BufferSize;
    IMMDeviceEnumerator* DeviceEnumerator;
    IMMDevice* WriteDevice;
    IAudioClient* WriteClient;
    IAudioRenderClient* RenderClient;
} win32_audio;


void
Win32ReleaseAudioWrite(win32_audio* Audio)
{
    if(Audio->RenderClient)
    {
        Audio->RenderClient->lpVtbl->Release(Audio->RenderClient);
        Audio->RenderClient = 0;
    }
    if(Audio->WriteClient)
    {
        Audio->WriteClient->lpVtbl->Release(Audio->WriteClient);
        Audio->WriteClient = 0;
    }
    if(Audio->WriteDevice)
    {
        Audio->WriteDevice->lpVtbl->Release(Audio->WriteDevice);
        Audio->WriteDevice = 0;
    }
}

b32
Win32StartDefaultAudioOutput(win32_audio* Audio)
{
    HRESULT Hr = Audio->DeviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(
        Audio->DeviceEnumerator, eRender, eConsole, &Audio->WriteDevice);
    Audio->Enabled = false;
    if(Hr == 0 && Audio->WriteDevice)
    {
        Hr = Audio->WriteDevice->lpVtbl->Activate(
            Audio->WriteDevice, IID_IAudioClient, CLSCTX_ALL, 0, (void**)&Audio->WriteClient);
        WAVEFORMATEX* WriteFormat = 0;
        if(Hr == 0 && Audio->WriteClient)
        {
            Hr = Audio->WriteClient->lpVtbl->GetMixFormat(Audio->WriteClient, &WriteFormat);
            if(Hr == 0 && WriteFormat)
            {
                WAVEFORMATEXTENSIBLE* Format        = (WAVEFORMATEXTENSIBLE*)WriteFormat;
                Format->Samples.wValidBitsPerSample = 32;
                Format->SubFormat                   = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
                WriteFormat->wBitsPerSample         = 32;
                WriteFormat->nBlockAlign            = (WriteFormat->wBitsPerSample >> 3) * WriteFormat->nChannels;
                WriteFormat->nAvgBytesPerSec =
                    WriteFormat->nSamplesPerSec * WriteFormat->nChannels * (WriteFormat->wBitsPerSample >> 3);
                Audio->SampleRate         = WriteFormat->nSamplesPerSec;
                Audio->SampleSize         = WriteFormat->wBitsPerSample >> 3;
                Audio->ChannelCount       = WriteFormat->nChannels;
                s64 SecondsToHundredNanos = 10000000;
                Hr                        = Audio->WriteClient->lpVtbl->Initialize(Audio->WriteClient,
                                                            AUDCLNT_SHAREMODE_SHARED,
                                                            0,
                                                            // NOTE(ronald): buffer size in 100-nanoseconds
                                                            (SecondsToHundredNanos) / 20,
                                                            0,
                                                            WriteFormat,
                                                            0);
                if(Hr == 0)
                {
                    u32 WriteBufferSize;
                    Hr = Audio->WriteClient->lpVtbl->GetBufferSize(Audio->WriteClient, &WriteBufferSize);
                    if(Hr == 0)
                    {
                        Audio->BufferSize = WriteBufferSize;
                        Hr                = Audio->WriteClient->lpVtbl->GetService(
                            Audio->WriteClient, IID_IAudioRenderClient, (void**)&Audio->RenderClient);
                        if(Hr == 0)
                        {
                            Hr = Audio->WriteClient->lpVtbl->Start(Audio->WriteClient);
                            if(Hr == 0)
                            {
                                Audio->Enabled = true;
                            }
                            else
                            {
                                Win32Log("Could not start audio render client");
                            }
                        }
                        else
                        {
                            Win32Log("Could not retrieve audio render client");
                        }
                    }
                    else
                    {
                        Win32Log("Could not get write client buffer size");
                    }
                }
                else
                {
                    Win32Log("Could not initialize write client");
                }
            }
            else
            {
                Win32Log("Could not retrieve mix format");
            }
        }
        else
        {
            Win32Log("Could not create audio write client");
        }
        if(WriteFormat)
        {
            CoTaskMemFree(WriteFormat);
        }
    }
    else
    {
        Win32Log("Could not create audio write device");
    }
    if(!Audio->Enabled)
    {
        Win32ReleaseAudioWrite(Audio);
        Win32Log("Failed to start audio device");
    }
    else
    {
        Win32Log("=== Audio ===");
        char PrintBuffer[1024];
        sprintf_s(PrintBuffer,
                  "SampleRate: %d\r\n"
                  "SampleSize: %d\r\n"
                  "ChannelCount: %d\r\n"
                  "BufferSize: %d\r\n",
                  Audio->SampleRate,
                  Audio->SampleSize,
                  Audio->ChannelCount,
                  Audio->BufferSize);
        Win32Log(PrintBuffer);
    }
    return Audio->Enabled;
}
