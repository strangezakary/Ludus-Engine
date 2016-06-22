/*
    Creation Date: 28-03-2016
    Creator: Florian Behr
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <locale.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>

// NOTE(inso): use older memcpy to reduce required glibc version
asm(".symver memcpy, memcpy@GLIBC_2.2.5");

#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#include "ludus_platform.h"
#include "ludus_renderer.cpp"

#include "linux_x11_ludus.h"

#include "linux_controller.cpp"
#include "linux_openal_audio.cpp"
#include "linux_x11_input.cpp"
#include "linux_x11_ogl.cpp"

b32 GlobalRunning;
b32 GlobalPause;
b32 GlobalDebugMode;
int GlobalLogHandle;
int GlobalDataDirHandle;
linuxX11_game_code GameCode;
linux_audio_backend GlobalAudioBackend;

internal void*
LinuxX11mmapSizeWrapper(u64 Size)
{
    Size += sizeof(u64);
    void* Base = mmap(0, Size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(Base == MAP_FAILED)
    {
        LINUX_FN_LOG("mmap: %s", strerror(errno));
        exit(1);
    }
    *(u64*)Base = Size;
    return (void*)((u8*)Base + sizeof(u64));
}

internal int
LinuxX11munmapSizeWrapper(void* Address)
{
    u64 Size = *(u64*)((u8*)Address - sizeof(u64));
    return munmap(Address, Size);
}

internal void
LinuxX11ProcessButtonPress(game_button_state* OldState, game_button_state* NewState, b32 IsDown)
{
    NewState->EndedDown           = IsDown;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal linuxX11_game_code
LinuxX11LoadGameCode(const char* SourceSoName, const char* LockFileName)
{
    linuxX11_game_code Result = {};
    struct stat Tmp;
    if(stat(LockFileName, &Tmp) == -1) // No lock file
    {
        LINUX_FN_LOG("Creating lock file [%s]", LockFileName);
        struct stat SoStat;
        if(stat(SourceSoName, &SoStat) == 0)
        {
            Result.SoLastWriteTime = SoStat.st_mtim;
            if((Result.SoHandle = dlopen(SourceSoName, RTLD_NOW)))
            {
                if((Result.GameUpdateAndRender =
                        (game_update_and_render*)dlsym(Result.SoHandle, "GameUpdateAndRender")) &&
                   (Result.GameGenerateAudioSamples =
                        (game_generate_audio_samples*)dlsym(Result.SoHandle, "GameGenerateAudioSamples")))
                {
                    Result.IsValid = true;
                }
                else
                {
                    LINUX_FN_LOG("%s: dlsym: %s", SourceSoName, dlerror());
                }
            }
            else
            {
                LINUX_FN_LOG("%s: dlopen: %s", SourceSoName, dlerror());
            }
        }
        else
        {
            LINUX_FN_LOG("%s: stat: %s", SourceSoName, strerror(errno));
        }
    }
    return Result;
}

internal void
LinuxX11UnloadGameCode(linuxX11_game_code* GameCode)
{
    if(GameCode->SoHandle)
    {
        dlclose(GameCode->SoHandle);
        GameCode->SoHandle = 0;
    }
    GameCode->IsValid                  = false;
    GameCode->GameUpdateAndRender      = 0;
    GameCode->GameGenerateAudioSamples = 0;
}

internal inline u64
LinuxX11GetMonotonicTime()
{
    u64 Result;
    struct timespec TmpTime;
    clock_gettime(CLOCK_MONOTONIC, &TmpTime);
    Result = TmpTime.tv_sec * Billion + TmpTime.tv_nsec;
    return Result;
}

PLATFORM_FREE_FILE_MEMORY(LinuxX11PlatformFreeFileMemory)
{
    if(Memory)
    {
        LinuxX11munmapSizeWrapper(Memory);
    }
}

PLATFORM_READ_ENTIRE_FILE(LinuxX11PlatfromReadEntireFile)
{
    read_file_result Result = {};
    int Handle;
    if((Handle = openat(GlobalDataDirHandle, FileName, O_RDONLY)) != -1)
    {
        struct stat FileStat;
        if(fstat(Handle, &FileStat) == 0)
        {
            Result.Contents = LinuxX11mmapSizeWrapper(FileStat.st_size);
            if(Result.Contents)
            {
                if(read(Handle, Result.Contents, FileStat.st_size) == FileStat.st_size)
                {
                    Result.ContentsSize = FileStat.st_size;
                }
                else
                {
                    LINUX_FN_LOG("%s: read: %s", FileName, strerror(errno));
                }
            }
            else
            {
                LINUX_FN_LOG("%s: empty contents", FileName);
            }
        }
        else
        {
            LINUX_FN_LOG("%s: fstat: %s", FileName, strerror(errno));
        }
        close(Handle);
    }
    else
    {
        LINUX_FN_LOG("%s: %s", FileName, strerror(errno));
    }

    return (Result);
}

PLATFORM_WRITE_ENTIRE_FILE(LinuxX11PlatformWriteEntireFile)
{
    b32 Result = false;
    int Handle;
    if((Handle = openat(GlobalDataDirHandle, FileName, O_CREAT | O_WRONLY, 0666)) != -1)
    {
        if(write(Handle, Memory, MemorySize) == MemorySize)
        {
            Result = true;
        }
        else
        {
            LINUX_FN_LOG("%s: write: %s", FileName, strerror(errno));
        }
    }
    else
    {
        LINUX_FN_LOG("%s: openat: %s", FileName, strerror(errno));
    }
    // fsync(Handle); // TODO stalls horribly, file reading/writing will need to be in a different thread to work right.
    close(Handle);
    return (Result);
}

PLATFORM_ALLOCATE_MEMORY(LinuxX11PlatformAllocateMemory)
{
    void* Result = LinuxX11mmapSizeWrapper(Size);

    return (Result);
}

PLATFORM_DEALLOCATE_MEMORY(LinuxX11PlatformDeallocatememory)
{
    if(Memory)
    {
        LinuxX11munmapSizeWrapper(Memory);
    }
}

PLATFORM_LOG(LinuxX11PlatformLog) // char* string
{
#ifdef LUDUS_INTERNAL
    puts(String);
#endif
    time_t Now = time(0);
    char TimeBuf[256];

    if(GlobalLogHandle != 0)
    {
        strftime(TimeBuf, sizeof(TimeBuf), "%F %T", localtime(&Now));
        dprintf(GlobalLogHandle, "[%s] %s\n", TimeBuf, String);
    }
}

PLATFORM_OUTPUT_DEBUG_STRING(LinuxX11OutputDebugString)
{
    u64 Length = StringLength(String);
    if(write(StdErr, String, Length) != Length)
    {
        Assert(!"This should always work, what is happening?");
    }
}

PLATFORM_GET_FILE_TIME(LinuxX11GetFileTime)
{
    s64 Result = 0;
    struct stat Stat;
    if(fstatat(GlobalDataDirHandle, FilePath, &Stat, 0) == 0)
    {
        // Roughly emulate Windows file time, which is Nanoseconds since epoch / 100
        // There are one Billion ns in one s, divided by 100 makes 10000000
        Result = Stat.st_mtim.tv_sec * 10000000 + Stat.st_mtim.tv_nsec / 100;
        // At some point either adjust this to account for the epoch, or document
        // that it's not the same across platforms. At which point we might as well
        // just reduce this to second resolution.
    }

    return Result;
}

void
LinuxLogF(const char* Fmt, ...)
{
    char Buf[4096];
    va_list V;

    va_start(V, Fmt);
    vsnprintf(Buf, sizeof(Buf), Fmt, V);
    LinuxX11PlatformLog(Buf);
    va_end(V);
}

internal void
LinuxLogSystemInfo(void)
{
    struct utsname UnameInfo = {};
    if(uname(&UnameInfo) != -1)
    {
        LinuxLogF("OS        : %s", UnameInfo.sysname);
        LinuxLogF("Release   : %s", UnameInfo.release);
        LinuxLogF("Version   : %s", UnameInfo.version);
        LinuxLogF("Machine   : %s", UnameInfo.machine);
    }
    else
    {
        LinuxLogF("uname: %s", strerror(errno));
    }

    char Line[256];
    char Distro[256];
    FILE* F = fopen("/etc/os-release", "r");
    if(F)
    {
        while(fgets(Line, sizeof(Line), F))
        {
            if(sscanf(Line, "PRETTY_NAME=%255[^\n]", Distro) == 1)
            {
                LinuxLogF("Distro    : %s", Distro);
                break;
            }
        }
        fclose(F);
    }

    u64 PageSize = sysconf(_SC_PAGESIZE);
    u64 TotalMem = sysconf(_SC_PHYS_PAGES) * PageSize;
    u64 AvailMem = sysconf(_SC_AVPHYS_PAGES) * PageSize;

    LinuxLogF("Total Mem : %d MB", TotalMem / (1024 * 1024));
    LinuxLogF("Avail Mem : %d MB", AvailMem / (1024 * 1024));
    LinuxLogF("CPU Cores : %d", sysconf(_SC_NPROCESSORS_ONLN));
    LinuxLogF("Locale    : %s", setlocale(LC_ALL, NULL));
    LinuxLogF("=====================");
}

internal void
LinuxX11ExitCleanup(int ReturnValue)
{
    fsync(GlobalLogHandle);
    if(GlobalAudioBackend == linux_audio_backend_openal)
    {
        LinuxOpenALShutdown();
    }
    _exit(ReturnValue);
};

void
LinuxX11ErrorExit(char* ErrorString)
{
    LinuxX11PlatformLog(ErrorString);
    LinuxX11ExitCleanup(1);
}

internal void
LinuxX11SignalHandler(int Signal)
{
    switch(Signal)
    {
        case SIGINT:
        {
            LinuxX11ExitCleanup(0);
        }
        break;
    }
}

#define LinuxX11ToggleFullscreen(Disp, Win) LinuxX11Fullscreen_(Disp, Win, 2)
#define LinuxX11SetFullscreen(Disp, Win) LinuxX11Fullscreen_(Disp, Win, 1)
#define LinuxX11RemoveFullscreen(Disp, Win) LinuxX11Fullscreen_(Disp, Win, 0)
Status
LinuxX11Fullscreen_(Display* Disp, Window Win, int ChangeAction)
{
    XClientMessageEvent ev = {};
    Atom WmState           = XInternAtom(Disp, "_NET_WM_STATE", false);
    Atom Fullscreen        = XInternAtom(Disp, "_NET_WM_STATE_FULLSCREEN", false);

    if(WmState == None)
        return 0;

    ev.type         = ClientMessage;
    ev.format       = 32;
    ev.window       = Win;
    ev.message_type = WmState;
    ev.data.l[0]    = ChangeAction;
    ev.data.l[1]    = Fullscreen;
    ev.data.l[2]    = false;
    ev.data.l[3]    = 1;

    return XSendEvent(
        Disp, DefaultRootWindow(Disp), 0, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&ev);
}

void
LinuxX11SetSizeHint(Display* Disp, Window Win, int MinWidth, int MinHeight, int MaxWidth, int MaxHeight)
{
    XSizeHints Hints = {};
    if(MinWidth > 0 && MinHeight > 0)
        Hints.flags |= PMinSize;
    if(MaxWidth > 0 && MaxHeight > 0)
        Hints.flags |= PMaxSize;

    Hints.min_width  = MinWidth;
    Hints.min_height = MinHeight;
    Hints.max_width  = MaxWidth;
    Hints.max_height = MaxHeight;

    XSetWMNormalHints(Disp, Win, &Hints);
}

#define LinuxX11EZKeyboardInput(Button, IsPressed)                                                                     \
    LinuxX11ProcessButtonPress(&OldInput->Controllers[0].Button, &NewInput->Controllers[0].Button, IsPressed)

#define LinuxX11EZMouseButtonInput(MouseButtonNum, IsPressed)                                                          \
    LinuxX11ProcessButtonPress(                                                                                        \
        &OldInput->MouseButtons[MouseButtonNum], &NewInput->MouseButtons[MouseButtonNum], IsPressed)

int
main(int argc, char** argv)
{
    signal(SIGINT, LinuxX11SignalHandler);

    char ProgPath[PATH_MAX] = {};
    ssize_t PathSize        = readlink("/proc/self/exe", ProgPath, sizeof(ProgPath));
    Assert(PathSize > 0);

    char* LastSlash = strrchr(ProgPath, '/');
    Assert(LastSlash);
    *LastSlash = 0;

    if(chdir(ProgPath) == -1)
    {
        LinuxLogF("chdir to %s failed: %s", ProgPath, strerror(errno));
        return 1;
    }

#if LUDUS_INTERNAL
    memcpy(LastSlash, "/../data/", 10);
    GlobalDataDirHandle = open(ProgPath, O_PATH | O_DIRECTORY);
    if(GlobalDataDirHandle == -1)
#endif
    {
        memcpy(LastSlash, "/data/", 7);
        GlobalDataDirHandle = open(ProgPath, O_PATH | O_DIRECTORY);
        if(GlobalDataDirHandle == -1)
        {
            GlobalDataDirHandle = AT_FDCWD;
        }
    }

    // TODO(inso): should we call this?
    setlocale(LC_ALL, "");

    GlobalLogHandle = open("log.txt", O_TRUNC | O_CREAT | O_WRONLY, 0666);
    Assert(GlobalLogHandle != -1);

    LinuxLogF("Ludus Engine Log File");
    LinuxLogF("=====================");
    LinuxLogSystemInfo();

    game_memory GameMemory                   = {};
    GameMemory.PermanentStorageSize          = Megabytes(256);
    GameMemory.TransientStorageSize          = Gigabytes(1);
    GameMemory.PlatformAPI.AllocateMemory    = LinuxX11PlatformAllocateMemory;
    GameMemory.PlatformAPI.DeallocateMemory  = LinuxX11PlatformDeallocatememory;
    GameMemory.PlatformAPI.FreeFileMemory    = LinuxX11PlatformFreeFileMemory;
    GameMemory.PlatformAPI.ReadEntireFile    = LinuxX11PlatfromReadEntireFile;
    GameMemory.PlatformAPI.WriteEntireFile   = LinuxX11PlatformWriteEntireFile;
    GameMemory.PlatformAPI.Log               = LinuxX11PlatformLog;
    GameMemory.PlatformAPI.OutputDebugString = LinuxX11OutputDebugString;
    GameMemory.PlatformAPI.GetFileTime       = LinuxX11GetFileTime;

    Platform = GameMemory.PlatformAPI;

    Display* Disp = XOpenDisplay(0);

    if(!Disp)
    {
        LinuxX11ErrorExit("No X display available; exiting");
    }

    XVisualInfo* VisInfo;
    GLXFBConfig* FBConfig;

    LinuxX11OpenGLInit(Disp, &VisInfo, &FBConfig);

    Window Root                     = DefaultRootWindow(Disp);
    XSetWindowAttributes WindowAttr = {};
    WindowAttr.colormap             = XCreateColormap(Disp, Root, VisInfo->visual, AllocNone);
    WindowAttr.event_mask =
        StructureNotifyMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    unsigned long AttributeMask = CWBackPixel | CWColormap | CWEventMask;

    Window Win = XCreateWindow(Disp,
                               Root,
                               0,
                               0,
                               WINDOW_WIDTH,
                               WINDOW_HEIGHT,
                               0,
                               VisInfo->depth,
                               InputOutput,
                               VisInfo->visual,
                               AttributeMask,
                               &WindowAttr);

    if(!Win)
    {
        LinuxX11ErrorExit("Window wasn't created properly; exiting");
    }

    XFree(VisInfo);
    VisInfo = NULL;

    Atom WM_DELETE_WINDOW = XInternAtom(Disp, "WM_DELETE_WINDOW", False);
    if(!XSetWMProtocols(Disp, Win, &WM_DELETE_WINDOW, 1))
    {
        LinuxX11PlatformLog("Couldn't register window manager WM_DELETE_WINDOW message, closing the Window might "
                            "unexpectedly kill the application");
    }
    XStoreName(Disp, Win, WINDOW_NAME);
    LinuxX11SetSizeHint(Disp, Win, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0);

    XMapWindow(Disp, Win);
    XFlush(Disp);

    // LinuxX11ToggleFullscreen(Disp, Win);

    GLXContext GLContext = LinuxX11OpenGLCreateContext(Disp, Win, FBConfig);
    InitOpenGL(true);

#ifdef LUDUS_INTERNAL
    void* BaseAddress = (void*)Terabytes(2);
#else
    void* BaseAddress = 0;
#endif

    u64 MemTotalSize      = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    void* GameMemoryBlock = mmap(BaseAddress, MemTotalSize, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(GameMemoryBlock == MAP_FAILED)
    {
        LinuxX11ErrorExit("Couldn't allocate game memory; exiting");
    }

    GameMemory.PermanentStorage = GameMemoryBlock;
    GameMemory.TransientStorage = ((u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

    render_buffer RenderBuffer = {};
    RenderBuffer.Size          = Megabytes(4);
    RenderBuffer.Base = (uint8*)mmap(0, RenderBuffer.Size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(RenderBuffer.Base == MAP_FAILED)
    {
        LinuxX11ErrorExit("Couldn't allocate render buffer memory; exiting");
    }

    if(GameMemoryBlock && GameMemory.PermanentStorage && GameMemory.TransientStorage && RenderBuffer.Base)
    {
        game_input GameInput[2] = {};
        game_input* OldInput    = GameInput;
        game_input* NewInput    = GameInput + 1;
        game_input* TmpInput;

        const char* GameSoName   = "./libgame.so";
        const char* LockFileName = "./lock";
        GameCode                 = LinuxX11LoadGameCode(GameSoName, LockFileName);

        LinuxControllerInit(NewInput->Controllers + 1);
        LinuxOpenALInit();

        u64 WorkTime;
        u64 LastTime               = LinuxX11GetMonotonicTime();
        u64 TargetFrameNanoSeconds = (u64)((1.0f / 60.0f) * Billion);

        GlobalRunning = true;
        while(GlobalRunning)
        {
#if LUDUS_INTERNAL
            GameMemory.ExecuteableReloaded = false;
            struct stat SoStat;
            if(stat(GameSoName, &SoStat) == 0)
            {
                if(SoStat.st_mtim.tv_sec != GameCode.SoLastWriteTime.tv_sec &&
                   SoStat.st_mtim.tv_nsec != GameCode.SoLastWriteTime.tv_nsec)
                {
                    LinuxX11UnloadGameCode(&GameCode);
                    GameCode                       = LinuxX11LoadGameCode(GameSoName, LockFileName);
                    GameMemory.ExecuteableReloaded = true;
                }
            }
            GameMemory.DebugMode = GlobalDebugMode;
#endif
            TmpInput                             = OldInput;
            OldInput                             = NewInput;
            NewInput                             = TmpInput;
            *NewInput                            = {};
            NewInput->Controllers[0].IsConnected = true;

            for(int i = 0; i < ArrayCount(OldInput->Controllers[0].Buttons); ++i)
            {
                NewInput->Controllers[0].Buttons[i].EndedDown = OldInput->Controllers[0].Buttons[i].EndedDown;
            }

            NewInput->MouseX = OldInput->MouseX;
            NewInput->MouseY = OldInput->MouseY;

            // TODO(inso): probably don't hard code this.
            NewInput->dtForFrame = (1.0f / 60.0f);

            XEvent ev = {};
            while(XPending(Disp) > 0)
            {
                XNextEvent(Disp, &ev);
                switch(ev.type)
                {
                    case DestroyNotify:
                    {
                        XDestroyWindowEvent* e = (XDestroyWindowEvent*)&ev;
                        if(e->window == Win)
                        {
                            GlobalRunning = false;
                        }
                    }
                    break;
                    case ClientMessage:
                    {
                        XClientMessageEvent* e = (XClientMessageEvent*)&ev;
                        if(e->data.l[0] == WM_DELETE_WINDOW)
                        {
                            XDestroyWindow(Disp, Win);
                            LinuxX11ExitCleanup(0);
                        }
                    }
                    break;
                    case MotionNotify:
                    {
                        XMotionEvent* e = (XMotionEvent*)&ev;
                        if(e->x && e->y)
                        {
                            NewInput->MouseX = e->x;
                            NewInput->MouseY = e->y;
                        }
                    }
                    break;
                    case ButtonPress:
                    {
                        XButtonPressedEvent* e = (XButtonPressedEvent*)&ev;
                        if(e->button == Button1)
                            LinuxX11EZMouseButtonInput(0, true);
                        else if(e->button == Button2)
                            LinuxX11EZMouseButtonInput(1, true);
                        else if(e->button == Button3)
                            LinuxX11EZMouseButtonInput(2, true);
                        else if(e->button == Button4)
                            LinuxX11EZMouseButtonInput(3, true);
                        else if(e->button == Button5)
                            LinuxX11EZMouseButtonInput(4, true);
                    }
                    break;
                    case ButtonRelease:
                    {
                        XButtonReleasedEvent* e = (XButtonReleasedEvent*)&ev;
                        if(e->button == Button1)
                            LinuxX11EZMouseButtonInput(0, false);
                        else if(e->button == Button2)
                            LinuxX11EZMouseButtonInput(1, false);
                        else if(e->button == Button3)
                            LinuxX11EZMouseButtonInput(2, false);
                        else if(e->button == Button4)
                            LinuxX11EZMouseButtonInput(3, false);
                        else if(e->button == Button5)
                            LinuxX11EZMouseButtonInput(4, false);
                    }
                    break;
                    case ConfigureNotify:
                    {
                        XConfigureEvent* e  = (XConfigureEvent*)&ev;
                        RenderBuffer.Width  = e->width;
                        RenderBuffer.Height = e->height;
                        OpenGLUpdateSize(e->width, e->height);
                    }
                    break;
                    case KeyPress:
                    {
                        XKeyPressedEvent* e = (XKeyPressedEvent*)&(ev);
                        KeySym Sym          = XLookupKeysym(e, 0);

                        NewInput->ControlDown = NewInput->ControlDown || (e->state & ControlMask);

                        if(Sym == XK_w)
                            LinuxX11EZKeyboardInput(MoveForward, true);
                        else if(Sym == XK_a)
                            LinuxX11EZKeyboardInput(MoveLeft, true);
                        else if(Sym == XK_s)
                            LinuxX11EZKeyboardInput(MoveBackward, true);
                        else if(Sym == XK_d)
                            LinuxX11EZKeyboardInput(MoveRight, true);
                        else if(Sym == XK_Up)
                            LinuxX11EZKeyboardInput(DPadUp, true);
                        else if(Sym == XK_Left)
                            LinuxX11EZKeyboardInput(DPadLeft, true);
                        else if(Sym == XK_Down)
                            LinuxX11EZKeyboardInput(DPadDown, true);
                        else if(Sym == XK_Right)
                            LinuxX11EZKeyboardInput(DPadRight, true);
                        else if(Sym == XK_Escape)
                            LinuxX11EZKeyboardInput(Start, true);

#if LUDUS_INTERNAL
                        else if(Sym == XK_p)
                        {
                            GlobalPause = !GlobalPause;
                        }
                        else if(Sym == XK_F1)
                        {
                            GlobalDebugMode = !GlobalDebugMode;
                        }
#endif
                    }
                    break;
                    case KeyRelease:
                    {
                        XKeyPressedEvent* e = (XKeyPressedEvent*)&(ev);
                        KeySym Sym          = XLookupKeysym(e, 0);

                        if(Sym == XK_w)
                            LinuxX11EZKeyboardInput(MoveForward, false);
                        else if(Sym == XK_a)
                            LinuxX11EZKeyboardInput(MoveLeft, false);
                        else if(Sym == XK_s)
                            LinuxX11EZKeyboardInput(MoveBackward, false);
                        else if(Sym == XK_d)
                            LinuxX11EZKeyboardInput(MoveRight, false);
                        else if(Sym == XK_Up)
                            LinuxX11EZKeyboardInput(DPadUp, false);
                        else if(Sym == XK_Left)
                            LinuxX11EZKeyboardInput(DPadLeft, false);
                        else if(Sym == XK_Down)
                            LinuxX11EZKeyboardInput(DPadDown, false);
                        else if(Sym == XK_Right)
                            LinuxX11EZKeyboardInput(DPadRight, false);
                        else if(Sym == XK_Escape)
                            LinuxX11EZKeyboardInput(Start, false);
                    }
                    break;
                }
            }

            memcpy(
                NewInput->Controllers + 1, OldInput->Controllers + 1, sizeof(game_controller_input) * NUM_CONTROLLERS);

            LinuxControllerUpdateAll(NewInput->Controllers + 1);

            RewindRenderBuffer(&RenderBuffer);
            if(GameCode.GameUpdateAndRender && !GlobalPause)
            {
                GameCode.GameUpdateAndRender(&GameMemory, NewInput, &RenderBuffer);
            }
            EndRenderBuffer(&RenderBuffer);


            if(GameCode.GameGenerateAudioSamples)
            {
                LinuxOpenALUpdate(&GameMemory);
            }

            OpenGLRenderBuffer(&RenderBuffer);
            glXSwapBuffers(Disp, Win);

            WorkTime      = LinuxX11GetMonotonicTime();
            u64 FrameTime = WorkTime - LastTime;
            if(FrameTime < TargetFrameNanoSeconds)
            {
                struct timespec SleepTime;
                struct timespec RemainingSleepTime;

                SleepTime.tv_sec  = 0;
                SleepTime.tv_nsec = TargetFrameNanoSeconds - FrameTime;
                while(nanosleep(&SleepTime, &RemainingSleepTime) == -1 && errno == EINTR)
                {
                    SleepTime = RemainingSleepTime;
                }
            }
            else
            {
                printf("Frame took longer than excpected: %fms\n", (r32)FrameTime / 1000000.0f);
            }

            u64 TmpTime = LinuxX11GetMonotonicTime();
            // printf("The frame took %fms\n", (r32)(TmpTime - LastTime) / 1000000.0f);
            LastTime = TmpTime;
        }
    }
    LinuxX11ExitCleanup(0);
    return 0;
}
