/*
    Creation Date: 31-03-2016
    Creator: Florian Behr
*/

#ifndef LINUXX11_LUDUS_H
#define LINUXX11_LUDUS_H

#define GLX_CREATE_CONTEXT_ATTRIBS_ARB(name)                                                                           \
    GLXContext name(Display* dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int* attrib_list)
typedef GLX_CREATE_CONTEXT_ATTRIBS_ARB(glx_create_context_attribs_arb);
global_variable glx_create_context_attribs_arb* glXCreateContextAttribsARB_ = NULL;
#define glXCreateContextAttribsARB glXCreateContextAttribsARB_

#define Billion 1000000000
#define StdOut 1
#define StdErr 2

#define AL_FORMAT_STEREO_FLOAT32 0x10011
#define ALC_CONNECTED 0x313

#define NUM_CONTROLLERS (ArrayCount(((game_input*)0)->Controllers) - 1)

#define LINUX_FN_LOG(Fmt, ...) LinuxLogF("[%s] " Fmt, __func__, ##__VA_ARGS__)

enum glx_flags
{
    FLAG_GLX_EXT_swap_control = 0x1
};

struct linuxX11_game_code
{
    game_update_and_render* GameUpdateAndRender;
    game_generate_audio_samples* GameGenerateAudioSamples;
    timespec SoLastWriteTime;
    void* SoHandle;
    b32 IsValid;
};

enum linux_audio_backend
{
    linux_audio_backend_none,
    linux_audio_backend_openal,
};

extern b32 GlobalRunning;
extern b32 GlobalPause;
extern b32 GlobalDebugMode;
extern int GlobalLogHandle;
extern int GlobalDataDirHandle;
extern linuxX11_game_code GameCode;
extern linux_audio_backend GlobalAudioBackend;

void
LinuxX11ErrorExit(char*);
void
LinuxX11PlatformLog(char*);
void
LinuxLogF(const char*, ...);

#endif
