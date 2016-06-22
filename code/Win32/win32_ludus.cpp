/*
    Creation Date: 26-03-2016
    Creator: Zakary Strange
*/

#include <windows.h>
#undef CopyMemory

#include <Shlwapi.h>

#include <dbt.h>
#include <gl\gl.h>
#include <stdio.h> // TODO(zak): Remove this later
#include <xinput.h>
#define CINTERFACE
#include <initguid.h>

#ifdef __cplusplus
#define HANDMADE_MATH_CPP_MODE
#endif
#define HANDMADE_MATH_IMPLEMENTATION
#include "..\HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#include "..\ludus_platform.h"
#include "..\ludus_renderer.cpp"

#define GL_LOAD_PROC wglGetProcAddress
#define GL_LOAD_PROC_CAST LPCSTR
#include "..\ludus_opengl.cpp"
#include "win32_ludus.h"

global_variable b32 GlobalRunning;
global_variable b32 GlobalPause;
global_variable b32 GlobalDebugMode;
global_variable HANDLE LogFileHandle;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

PLATFORM_LOG(Win32Log)
{

    if(LogFileHandle)
    {
        SetFilePointer(LogFileHandle, 0, 0, FILE_END);

        DWORD BytesWritten;
        WriteFile(LogFileHandle, String, (DWORD)StringLength(String), &BytesWritten, 0);
        if(BytesWritten != StringLength(String))
        {
            Assert("BytesWritten is not the size that should've been written");
        }

        BytesWritten = 0;
        WriteFile(LogFileHandle, "\r\n", 2, &BytesWritten, 0);
    }
}

#define LOG_FORMATTED_ERROR(size)                                                                                      \
    char _ErrorString[size];                                                                                           \
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, _ErrorString, sizeof(_ErrorString), 0);            \
    Win32Log(_ErrorString);


internal void
Win32ToggleFullscreen(HWND Window)
{
    // NOTE(zak): This follows Raymond Chen's prescription
    // for fullscreen toggling, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx

    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window,
                         HWND_TOP,
                         MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(
            Window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

#include "win32_input.cpp"
#include "win32_audio.cpp"
#include "win32_ogl.cpp"

#define MAX_CONTROLLER_COUNT 4

global_variable s64 GlobalPerfCountFrequency;

global_variable b32 XInputControllerActive[MAX_CONTROLLER_COUNT];
global_variable b32 UpdateXInputControllers;
global_variable win32_audio Win32Audio;

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

global_variable uint32 WindowWidth  = WINDOW_WIDTH;
global_variable uint32 WindowHeight = WINDOW_HEIGHT;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return (Result);
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);

    return (Result);
}

inline FILETIME
Win32GetLastWriteTime(char* FileName)
{
    FILETIME LastWriteTime = {0};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(FileName, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return (LastWriteTime);
}

internal void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_1.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }
    }
    else
    {
        Win32Log("Couldn't get any version of XInput");
    }
}

internal void
Win32GetEXEFileName(win32_state* State)
{
    DWORD SizeOfFileName               = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;

    for(char* Scan = State->EXEFileName; *Scan; ++Scan)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

internal void
Win32BuildEXEPathFileName(win32_state* State, char* FileName, int DestCount, char* Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName,
               State->EXEFileName,
               StringLength(FileName),
               FileName,
               DestCount,
               Dest);
}


PLATFORM_FREE_FILE_MEMORY(Win32PlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

PLATFORM_READ_ENTIRE_FILE(Win32PlatfromReadEntireFile)
{
    read_file_result Result = {0};

    HANDLE File = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(File, &FileSize))
        {
            u32 FileSize32  = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(File, Result.Contents, FileSize32, &BytesRead, 0) && BytesRead == FileSize32)
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    Win32Log(FileName);
                    Win32Log("BytesRead was not equal FileSize32");
                    Win32PlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                Win32Log(FileName);
                Win32Log("Couldnt allocate memory for file read");
            }
        }
        else
        {
            Win32Log(FileName);
            Win32Log("Coudlnt get file size");
            LOG_FORMATTED_ERROR(4096);
        }


        CloseHandle(File);
    }
    else
    {
        Win32Log(FileName);
        Win32Log("Couldnt create file handle");
        LOG_FORMATTED_ERROR(4096);
    }

    return (Result);
}

PLATFORM_WRITE_ENTIRE_FILE(Win32PlatformWriteEntireFile)
{
    b32 Result = false;

    HANDLE File = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(File != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(File, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            Win32Log(FileName);
            Win32Log("Could not write memory into file");
        }


        CloseHandle(File);
    }
    else
    {
        Win32Log(FileName);
        Win32Log("Could not create file name");
        LOG_FORMATTED_ERROR(4096);
    }

    return (Result);
}

PLATFORM_OUTPUT_DEBUG_STRING(Win32PlatformOutputDebugString)
{
    OutputDebugString(String);
}

PLATFORM_GET_FILE_TIME(Win32PlatformGetFileTime)
{
    s64 FileWriteTime = 0;

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(FilePath, GetFileExInfoStandard, &Data))
    {
        FileWriteTime = (((s64)Data.ftLastWriteTime.dwHighDateTime << 32) + Data.ftLastWriteTime.dwLowDateTime);
    }

    return (FileWriteTime);
}

PLATFORM_ALLOCATE_MEMORY(Win32PlatformAllocateMemory)
{
    void* Result = VirtualAlloc(0, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    return (Result);
}

PLATFORM_DEALLOCATE_MEMORY(Win32PlatformDeallocatememory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal win32_game_code
Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName)
{
    win32_game_code Result = {0};

#if LUDUS_INTERNAL
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);

        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if(Result.GameCodeDLL)
        {
            Result.GameUpdateAndRender =
                (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.GameGenerateAudioSamples =
                (game_generate_audio_samples*)GetProcAddress(Result.GameCodeDLL, "GameGenerateAudioSamples");

            // NOTE(zak): In the future test each function your pulling from the dll
            Result.IsValid = Result.GameUpdateAndRender && Result.GameGenerateAudioSamples;
        }
        else
        {
            Win32Log("Could not load game.dll");
        }
    }
#else

    Result.GameCodeDLL = LoadLibraryA(SourceDLLName);
    if(Result.GameCodeDLL)
    {
        Result.GameUpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

        Result.GameGenerateAudioSamples =
            (game_generate_audio_samples*)GetProcAddress(Result.GameCodeDLL, "GameGenerateAudioSamples");

        Result.IsValid = Result.GameUpdateAndRender && Result.GameGenerateAudioSamples;
    }
#endif

    if(!Result.IsValid)
    {
        Result.GameUpdateAndRender      = 0;
        Result.GameGenerateAudioSamples = 0;
    }

    return (Result);
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid                  = false;
    GameCode->GameUpdateAndRender      = 0;
    GameCode->GameGenerateAudioSamples = 0;
}

internal void
Win32GetSystemInfo(void)
{
    int32 WindowsMajorVersion = 0;
    int32 WindowsMinorVersion = 0;
    int32 WindowsBuildNumber  = 0;

    char KernelPath[4096];
    GetSystemDirectory(KernelPath, sizeof(KernelPath));
    PathAppend(KernelPath, "kernel32.dll");

    DWORD KernelFileVersionInfoSize = GetFileVersionInfoSize(KernelPath, 0);

    DWORD Error = GetLastError();

    if(KernelFileVersionInfoSize != 0)
    {
        void* KernelVersionInfo = Win32PlatformAllocateMemory(KernelFileVersionInfoSize);

        if(GetFileVersionInfo(KernelPath, 0, KernelFileVersionInfoSize, KernelVersionInfo))
        {
            VS_FIXEDFILEINFO* KernelVersionFileInfo = 0;
            UINT KernelVersionFileInfoLength        = 0;
            if(VerQueryValue(KernelVersionInfo, "\\", (LPVOID*)&KernelVersionFileInfo, &KernelVersionFileInfoLength))
            {
                WindowsMajorVersion = HIWORD(KernelVersionFileInfo->dwProductVersionMS);
                WindowsMinorVersion = LOWORD(KernelVersionFileInfo->dwProductVersionMS);

                if(WindowsMinorVersion == 5)
                {
                    if(WindowsMinorVersion == 0)
                    {
                        MessageBoxA(0,
                                    "This application requires at least Windows Vista to run",
                                    "Windows Version Error",
                                    MB_OK);
                        ExitProcess(1);
                    }
                    else if(WindowsMinorVersion == 1)
                    {
                        MessageBoxA(0,
                                    "This application requires at least Windows Vista to run",
                                    "Windows Version Error",
                                    MB_OK);
                        ExitProcess(1);
                    }
                }

                // TODO(zak): When we start supporting 32-bit add a check if the user is running x86 or x64
                if(WindowsMinorVersion == 6)
                {
                    if(WindowsMinorVersion == 0)
                    {
                        Win32Log("Operating System: Windows Vista");
                    }
                    else if(WindowsMinorVersion == 1)
                    {
                        Win32Log("Operating System: Windows 7");
                    }

                    else if(WindowsMinorVersion == 2)
                    {
                        Win32Log("Operating System: Windows 8");
                    }
                    else if(WindowsMinorVersion == 3)
                    {
                        Win32Log("Operating System: Windows 8.1");
                    }
                }

                if(WindowsMajorVersion == 10)
                {
                    Win32Log("Operating System: Windows 10");
                }
            }
            else
            {
                Win32Log("Version info could not be found");
            }
        }
        else
        {
            Win32Log("Could not successfully GetFileVersionInfo on KernelVersionInfo");
        }
    }
    else
    {
        Win32Log("Could not query KernelFileVersionInfoSize");
    }

    Win32Log("===  CPU  ====");
    SYSTEM_INFO SystemInfo = {0};
    GetSystemInfo(&SystemInfo);

    if(SystemInfo.dwProcessorType == 386)
    {
        Win32Log("Processor Type: Intel 386");
    }
    else if(SystemInfo.dwProcessorType == 486)
    {
        Win32Log("Processor Type: Intel 486");
    }
    else if(SystemInfo.dwProcessorType == 586)
    {
        Win32Log("Processor Type: Intel 586 (Pentium)");
    }
    else if(SystemInfo.dwProcessorType == 2200)
    {
        Win32Log("Processor Type: Intel IA64");
    }
    else if(SystemInfo.dwProcessorType == 8664)
    {
        Win32Log("Processor Type: AMD x8664");
    }
    else
    {
        Win32Log("ARM or Undefined");
    }

    if(SystemInfo.wProcessorArchitecture == 9)
    {
        Win32Log("Processor Architecture: x64");
    }
    else if(SystemInfo.wProcessorArchitecture == 5)
    {
        Win32Log("Processor Architecture: ARM");
    }
    else if(SystemInfo.wProcessorArchitecture == 6)
    {
        Win32Log("Processor Architecture: Intel Itanium-based");
    }
    else if(SystemInfo.wProcessorArchitecture == 0)
    {
        Win32Log("Processor Architecture: x86");
    }
    else if(SystemInfo.wProcessorArchitecture == 0xffff)
    {
        Win32Log("Processor Architecture: Unknown architecture");
    }

    // TODO(Hoej): Investigate if __CPUID is reliable
    char CoreBuffer[60];
    sprintf_s(CoreBuffer, "Number of Logical Cores: %d", SystemInfo.dwNumberOfProcessors);
    Win32Log(CoreBuffer);

    MEMORYSTATUSEX MemoryStatus = {0};
    MemoryStatus.dwLength       = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&MemoryStatus);

    Win32Log("=== Memory ===");
    char PhysicalMemoryBuffer[256];
    sprintf_s(PhysicalMemoryBuffer,
              "Physical Memory: %llu bytes \nIn Use: %llu bytes (%lu%%) \nFree: %llu bytes",
              MemoryStatus.ullTotalPhys,
              MemoryStatus.ullTotalPhys - MemoryStatus.ullAvailPhys,
              MemoryStatus.dwMemoryLoad,
              MemoryStatus.ullAvailPhys);
    Win32Log(PhysicalMemoryBuffer);
}

LRESULT CALLBACK
MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_ACTIVATEAPP:
        {
            // TODO(zak): Handle this
        }
        break;

        case WM_SIZE:
        {
            WindowWidth  = LOWORD(LParam);
            WindowHeight = HIWORD(LParam);

#if USE_OGL_RENDERER
            OpenGLUpdateSize(WindowWidth, WindowHeight);
#endif
        }
        break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        }
        break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
        }
        break;

        case WM_DEVICECHANGE:
        {
            UpdateXInputControllers = true;
        }
        break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        }
        break;
    }

    return (Result);
}

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    LogFileHandle = CreateFileA("log.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    SetCurrentDirectory("data");
    Win32Log("Ludus Engine Log File");
    Win32Log("==============");

    Win32GetSystemInfo();

    LARGE_INTEGER PerCountFrequencyResult;
    QueryPerformanceFrequency(&PerCountFrequencyResult);
    GlobalPerfCountFrequency = PerCountFrequencyResult.QuadPart;

    win32_state Win32State = {0};

    Win32GetEXEFileName(&Win32State);

    char SourceGameCodeDLLFullPath[4096];
    Win32BuildEXEPathFileName(&Win32State, "game.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[4096];
    Win32BuildEXEPathFileName(&Win32State, "game_temp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[4096];
    Win32BuildEXEPathFileName(&Win32State, "lock.tmp", sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGradular     = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();

    WNDCLASSA WindowClass     = {0};
    WindowClass.style         = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc   = MainWindowCallback;
    WindowClass.hInstance     = Instance;
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszClassName = "LudusWindowClass";

    if(RegisterClassA(&WindowClass))
    {
        RECT WindowRect   = {};
        WindowRect.left   = 0;
        WindowRect.right  = WINDOW_WIDTH;
        WindowRect.top    = 0;
        WindowRect.bottom = WINDOW_HEIGHT;
        AdjustWindowRect(&WindowRect, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0);

        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      WINDOW_NAME,
                                      WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      WindowRect.right - WindowRect.left,
                                      WindowRect.bottom - WindowRect.top,
                                      0,
                                      0,
                                      Instance,
                                      0);

        if(Window)
        {
            DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {0};
            GUID USB_GUID = {0x25dbce51, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35};

            NotificationFilter.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            NotificationFilter.dbcc_classguid  = USB_GUID;

            ZeroMemory(&XInputControllerActive, sizeof(XInputControllerActive));
            UpdateXInputControllers = true;
            HDEVNOTIFY USBDeviceNotify =
                RegisterDeviceNotification(Window, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
            if(USBDeviceNotify == 0)
            {
                Win32Log("Could not register for USB device notifications");
            }

            int MonitorRefreshHz = 60;
            HDC RefreshDC        = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            r32 GameUpdateHz          = (r32)(MonitorRefreshHz);
            r32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

#ifdef LUDUS_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory                   = {0};
            GameMemory.PermanentStorageSize          = Megabytes(256);
            GameMemory.TransientStorageSize          = Gigabytes(1);
            GameMemory.PlatformAPI.AllocateMemory    = Win32PlatformAllocateMemory;
            GameMemory.PlatformAPI.DeallocateMemory  = Win32PlatformDeallocatememory;
            GameMemory.PlatformAPI.FreeFileMemory    = Win32PlatformFreeFileMemory;
            GameMemory.PlatformAPI.ReadEntireFile    = Win32PlatfromReadEntireFile;
            GameMemory.PlatformAPI.WriteEntireFile   = Win32PlatformWriteEntireFile;
            GameMemory.PlatformAPI.Log               = Win32Log;
            GameMemory.PlatformAPI.OutputDebugString = Win32PlatformOutputDebugString;
            GameMemory.PlatformAPI.GetFileTime       = Win32PlatformGetFileTime;

            Platform = GameMemory.PlatformAPI;

            Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

            Win32State.GameMemoryBlock =
                VirtualAlloc(BaseAddress, (memory_index)Win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

            render_buffer RenderBuffer = {};
            RenderBuffer.Size          = Megabytes(4);
            RenderBuffer.Base = (uint8*)VirtualAlloc(0, RenderBuffer.Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            if(GameMemory.PermanentStorage && GameMemory.TransientStorage && RenderBuffer.Base)
            {
#if USE_OGL_RENDERER
                HDC WindowDC = GetDC(Window);
                Win32InitOpenGL(WindowDC);
                ReleaseDC(Window, WindowDC);
#endif

                HRESULT Hr = CoInitializeEx(0, COINIT_MULTITHREADED);
                if(Hr == 0)
                {
                    Hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
                                          0,
                                          CLSCTX_ALL,
                                          IID_IMMDeviceEnumerator,
                                          (void**)&Win32Audio.DeviceEnumerator);

                    if(Hr == 0 && Win32Audio.DeviceEnumerator)
                    {
                        Win32StartDefaultAudioOutput(&Win32Audio);
                    }
                    else
                    {
                        Win32Log("Could not create audio device enumerator");
                    }
                }
                else
                {
                    Win32Log("Could not initialize COM on main thread");
                }

                game_input Input[2]  = {0};
                game_input* OldInput = &Input[0];
                game_input* NewInput = &Input[1];

                LARGE_INTEGER LastCounter   = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                win32_game_code Game =
                    Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);

                GlobalRunning = true;

                while(GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;

#ifdef LUDUS_INTERNAL
                    GameMemory.ExecuteableReloaded = false;
                    FILETIME NewDLLWriteTime       = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game =
                            Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);

                        GameMemory.ExecuteableReloaded = true;
                    }
                    GameMemory.DebugMode = GlobalDebugMode;
#endif
                    game_controller_input* OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input* NewKeyboardController = GetController(NewInput, 0);
                    ZeroMemory((void*)NewKeyboardController, sizeof(game_controller_input));
                    NewKeyboardController->IsConnected = true;

                    for(u64 ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    if(!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = (r32)MouseP.x;
                        NewInput->MouseY = (r32)MouseP.y;

                        NewInput->ShiftDown   = (GetKeyState(VK_SHIFT) & (1 << 15));
                        NewInput->AltDown     = (GetKeyState(VK_MENU) & (1 << 15));
                        NewInput->ControlDown = (GetKeyState(VK_CONTROL) & (1 << 15));

                        DWORD WinButtonID[PlatformMouseButton_Count] = {
                            VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2,
                        };

                        for(u32 ButtonIndex = 0; ButtonIndex < PlatformMouseButton_Count; ++ButtonIndex)
                        {
                            NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                            NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
                                                        GetKeyState(WinButtonID[ButtonIndex]) & (1 << 15));
                        }

                        DWORD MaxControllerCount = MAX_CONTROLLER_COUNT;
                        if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                        }

                        if(UpdateXInputControllers)
                        {
                            for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
                            {
                                if(XInputControllerActive[ControllerIndex])
                                {
                                    continue;
                                }

                                XINPUT_STATE ControllerState;
                                XInputControllerActive[ControllerIndex] =
                                    (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS);
                            }
                            UpdateXInputControllers = false;
                            OutputDebugString("Updating controller activeness\n");
                        }
                        LARGE_INTEGER XInputCounter = Win32GetWallClock();
                        for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
                        {
                            DWORD OurControllerIndex             = ControllerIndex + 1;
                            game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input* NewController = GetController(NewInput, OurControllerIndex);
                            if(XInputControllerActive[ControllerIndex])
                            {
                                XINPUT_STATE ControllerState;
                                if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                                {
                                    NewController->IsConnected = true;
                                    NewController->IsAnalog    = OldController->IsAnalog;

                                    XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
                                    NewController->LeftStickAverageX =
                                        Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    NewController->LeftStickAverageY =
                                        Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    NewController->RightStickAverageX = Win32ProcessXInputStickValue(
                                        Pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                                    NewController->RightStickAverageY = Win32ProcessXInputStickValue(
                                        Pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

                                    if((NewController->LeftStickAverageX != 0.0f) ||
                                       (NewController->LeftStickAverageY != 0.0f) ||
                                       (NewController->RightStickAverageX != 0.0f) ||
                                       (NewController->RightStickAverageY != 0.0f))
                                    {
                                        NewController->IsAnalog = true;
                                    }

                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->DPadUp,
                                                                    XINPUT_GAMEPAD_DPAD_UP,
                                                                    &NewController->DPadUp);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->DPadDown,
                                                                    XINPUT_GAMEPAD_DPAD_DOWN,
                                                                    &NewController->DPadDown);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->DPadLeft,
                                                                    XINPUT_GAMEPAD_DPAD_LEFT,
                                                                    &NewController->DPadLeft);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->DPadRight,
                                                                    XINPUT_GAMEPAD_DPAD_RIGHT,
                                                                    &NewController->DPadRight);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ButtonDown,
                                                                    XINPUT_GAMEPAD_A,
                                                                    &NewController->ButtonDown);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ButtonRight,
                                                                    XINPUT_GAMEPAD_B,
                                                                    &NewController->ButtonRight);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ButtonLeft,
                                                                    XINPUT_GAMEPAD_X,
                                                                    &NewController->ButtonLeft);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->ButtonUp,
                                                                    XINPUT_GAMEPAD_Y,
                                                                    &NewController->ButtonUp);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->Start,
                                                                    XINPUT_GAMEPAD_START,
                                                                    &NewController->Start);
                                    Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                                    &OldController->Select,
                                                                    XINPUT_GAMEPAD_BACK,
                                                                    &NewController->Select);
                                }
                                else
                                {
                                    // NOTE(zak): No controller is avaliable
                                    NewController->IsConnected              = false;
                                    XInputControllerActive[ControllerIndex] = false;
                                }
                            }
                            else
                            {
                                NewController->IsConnected = false;
                            }
                        }
#if 0
                        LARGE_INTEGER XInputEndCounter = Win32GetWallClock();
                        r32 InputuS = 1000000.0f * Win32GetSecondsElapsed(XInputCounter, XInputEndCounter);
                        char InputuSFrameBuffer[256];
                        _snprintf_s(InputuSFrameBuffer, sizeof(InputuSFrameBuffer), "XInput Update Time: %.02fus\n", InputuS);
                        OutputDebugStringA(InputuSFrameBuffer);
#endif

                        RewindRenderBuffer(&RenderBuffer);
                        if(Game.GameUpdateAndRender)
                        {
                            RenderBuffer.Width  = WindowWidth;
                            RenderBuffer.Height = WindowHeight;
                            Game.GameUpdateAndRender(&GameMemory, NewInput, &RenderBuffer);
                        }
                        EndRenderBuffer(&RenderBuffer);
                        if(Win32Audio.DeviceLost)
                        {
                            Win32ReleaseAudioWrite(&Win32Audio);
                            Win32StartDefaultAudioOutput(&Win32Audio);
                            Win32Audio.DeviceLost = false;
                        }
                        if(Win32Audio.Enabled)
                        {
                            u32 Padding;
                            b32 Success = false;
                            Hr = Win32Audio.WriteClient->lpVtbl->GetCurrentPadding(Win32Audio.WriteClient, &Padding);
                            if(Hr == 0)
                            {
                                s32 WriteAmmount = Win32Audio.BufferSize - Padding;
                                BYTE* Buffer;
                                if(WriteAmmount <= Win32Audio.BufferSize && WriteAmmount > 0)
                                {
                                    Hr = Win32Audio.RenderClient->lpVtbl->GetBuffer(
                                        Win32Audio.RenderClient, WriteAmmount, &Buffer);
                                    if(Hr == 0)
                                    {
                                        ZeroMemory(Buffer, WriteAmmount * sizeof(float) * Win32Audio.ChannelCount);
                                        audio_sample_request SampleRequest = {0};
                                        SampleRequest.SampleRate           = Win32Audio.SampleRate;
                                        SampleRequest.ChannelCount         = Win32Audio.ChannelCount;
                                        SampleRequest.SampleCount          = WriteAmmount;
                                        SampleRequest.SampleBuffer         = (float*)Buffer;
                                        if(Game.GameGenerateAudioSamples)
                                        {
                                            Game.GameGenerateAudioSamples(&GameMemory, &SampleRequest);
                                        }
                                        Hr = Win32Audio.RenderClient->lpVtbl->ReleaseBuffer(
                                            Win32Audio.RenderClient, WriteAmmount, 0);
                                        if(Hr == 0)
                                        {
                                            Success = true;
                                        }
                                    }
                                }
                                else
                                {
                                    Success = true;
                                }
                            }
                            if(!Success)
                            {
                                Win32Audio.DeviceLost = true;
                                Win32Log("Device lost, trying to refresh");
                            }                        
                        }

#if USE_OGL_RENDERER
                        OpenGLRenderBuffer(&RenderBuffer);
#endif

                        HDC SwapBufferDC = GetDC(Window);
                        SwapBuffers(SwapBufferDC);
                        ReleaseDC(Window, SwapBufferDC);

                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        real32 SecondsElaspedForFrame = WorkSecondsElapsed;
                        if(SecondsElaspedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGradular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElaspedForFrame));

                                // NOTE(kiljacken): We can't trust the sleep granularity, so sleep one unit less than we
                                // need,
                                // to give the operating system some leeway.
                                if(SleepMS > DesiredSchedulerMS)
                                {
                                    Sleep(SleepMS - DesiredSchedulerMS);
                                }
                            }

                            real32 TestSecondsElapsedForFrame =
                                Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

                            if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                // TODO(zak): we didnt sleep when we should've
                            }

                            while(SecondsElaspedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElaspedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // NOTE(zak): We missed a frame
                        }

                        FlipWallClock = Win32GetWallClock();

                        game_input* Temp = NewInput;
                        NewInput         = OldInput;
                        OldInput         = Temp;

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        r32 MSPerFrame           = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter              = EndCounter;

#if LUDUS_INTERNAL
                        char MSPerFrameBuffer[256];
                        sprintf_s(MSPerFrameBuffer, "%.02f ms/f \n", MSPerFrame);
                        OutputDebugStringA(MSPerFrameBuffer);
#endif
                    }
                }

                CloseHandle(LogFileHandle);
            }
            else
            {
                Win32Log("Couldnt allocate memory for the game");
            }
        }
        else
        {
            Win32Log("Couldnt create main window");
        }
    }
    else
    {
        Win32Log("Couldnt register class for the main window");
    }


    return (0);
}
