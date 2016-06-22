/*
    Creation Date: 01-04-2016
    Creator: Andrew Spiering
*/

#include <Cocoa/Cocoa.h>
#import <CoreVideo/CVDisplayLink.h>
#import <IOKit/hid/IOHIDLib.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>


#include <asl.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libproc.h>
#import <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#undef MIN
#undef MAX
#undef ABS

// OSX doesn't have a GetProc like other OS's do for openGL
// So we have to do it ourselves
static void* glDLL = NULL;
void*
GLGetProcAddress(const GLubyte* name)
{
    void* proc = NULL;
    if(glDLL == NULL)
    {
        // having to put the full path here sucks :/
        glDLL = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);
    }
    if(glDLL)
    {
        proc = dlsym(glDLL, (char*)name);
    }
    return proc;
}

#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_IMPLEMENTATION
#include "../HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#include "../ludus_platform.h"
#include "../ludus_renderer.cpp"

#define GL_LOAD_PROC GLGetProcAddress
#define GL_LOAD_PROC_CAST const GLubyte*
#include "../ludus_opengl.cpp"
#include "osx_ludus.h"

internal void
OSXLogHelper(int level, const char* fmt, va_list ap)
{
    aslmsg msg;
    static aslclient logClient = (aslclient)0;

    if(!logClient)
    {
        logClient = asl_open(NULL, "com.apple.console", 0);
    }
    if(logClient)
    {
        asl_vlog(logClient, NULL, level, fmt, ap);
    }
}

#define __OSXLOG_VARGS_LOG(LEVEL)                                                                                      \
    va_list ap;                                                                                                        \
    va_start(ap, fmt);                                                                                                 \
    OSXLogHelper(LEVEL, fmt, ap);                                                                                      \
    va_end(ap);

inline void
OSX_LOG(const char* fmt, ...)
{
    __OSXLOG_VARGS_LOG(ASL_LEVEL_NOTICE);
}

#define OSX_LOG(...)

PLATFORM_LOG(OSXPlatformLog) // char* string
{
    if(!String)
        return;

    NSString* log = [[NSString alloc] initWithUTF8String:String];
    NSLog(@"%@", log);

    OSX_LOG(String);
}

internal void*
OSXmmapSizeWrapper(u64 Size)
{
    Size += sizeof(u64);
    void* Base = mmap(0, Size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(Base == MAP_FAILED)
    {
        // $AS TODO LOG ME
        exit(1);
    }
    *(u64*)Base = Size;
    return (void*)((u8*)Base + sizeof(u64));
}

internal int
OSXmunmapSizeWrapper(void* Address)
{
    u64 Size = *(u64*)((u8*)Address - sizeof(u64));
    return munmap(Address, Size);
}


internal osx_game_code
OsxLoadGameCode(char* SourceDLLName, char* LockFileName)
{
    osx_game_code Result = {0};
    struct stat FileStat;
    if(stat(SourceDLLName, &FileStat) == 0)
    {
        Result.DLLastWriteTime = FileStat.st_mtimespec.tv_sec;
        Result.GameCodeDLL     = dlopen(SourceDLLName, RTLD_LAZY | RTLD_GLOBAL);
        if(Result.GameCodeDLL)
        {
            Result.GameUpdateAndRender = (game_update_and_render*)dlsym(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.IsValid = (Result.GameUpdateAndRender != NULL);
        }
        else
        {
            Result.IsValid             = false;
            Result.GameUpdateAndRender = 0;
        }
    }
    return Result;
}

internal void
OSXUnloadGameCode(osx_game_code* GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        dlclose(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    GameCode->IsValid             = false;
    GameCode->GameUpdateAndRender = 0;
}

void
OSXGetEXEFileName(osx_state* State)
{
    pid_t PID = getpid();
    int r     = proc_pidpath(PID, State->EXEFileName, sizeof(State->EXEFileName));

    if(r <= 0)
    {
        fprintf(stderr, "Error getting process path: pid %d: %s\n", PID, strerror(errno));
    }
    else
    {
        OSX_LOG("process pid: %d   path: %s\n", PID, State->EXEFileName);
    }

    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for(char* Scan = State->EXEFileName; *Scan; ++Scan)
    {
        if(*Scan == '/')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

void
OSXBuildEXEPathFileName(osx_state* State, char* Filename, int DestCount, char* Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName,
               State->EXEFileName,
               StringLength(Filename),
               Filename,
               DestCount,
               Dest);
}

PLATFORM_ALLOCATE_MEMORY(OSXPlatformAllocateMemory)
{
    void* Result = OSXmmapSizeWrapper(Size);

    return (Result);
}

PLATFORM_DEALLOCATE_MEMORY(OSXPlatformDeallocateMemory)
{
    if(Memory)
    {
        OSXmunmapSizeWrapper(Memory);
    }
}

PLATFORM_FREE_FILE_MEMORY(OSXPlatformFreeFileMemory)
{
    if(Memory)
    {
        OSXmunmapSizeWrapper(Memory);
    }
}

PLATFORM_READ_ENTIRE_FILE(OSXPlatfromReadEntireFile)
{
    read_file_result Result = {};
    int Handle;
    if((Handle = open(FileName, O_RDONLY)) != -1)
    {
        struct stat FileStat;
        if(fstat(Handle, &FileStat) == 0)
        {
            Result.Contents = OSXmmapSizeWrapper(FileStat.st_size);
            if(Result.Contents)
            {
                if(read(Handle, Result.Contents, FileStat.st_size) == FileStat.st_size)
                {
                    Result.ContentsSize = FileStat.st_size;
                }
                else
                {
                    OSX_LOG("%s: read: %s", FileName, strerror(errno));
                }
            }
            else
            {
                OSX_LOG("%s: empty contents", FileName);
            }
        }
        else
        {
            OSX_LOG("%s: fstat: %s", FileName, strerror(errno));
        }
        close(Handle);
    }
    else
    {
        OSX_LOG("%s: %s", FileName, strerror(errno));
    }

    return (Result);
}

PLATFORM_WRITE_ENTIRE_FILE(OSXPlatformWriteEntireFile)
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
            OSX_LOG("%s: write: %s", FileName, strerror(errno));
        }
    }
    else
    {
        OSX_LOG("%s: openat: %s", FileName, strerror(errno));
    }
    close(Handle);
    return (Result);
}

PLATFORM_OUTPUT_DEBUG_STRING(OSXOutputDebugString)
{
    u64 Length = StringLength(String);
    if(write(StdErr, String, Length) != Length)
    {
        Assert(!"This should always work, what is happening?");
    }
}

PLATFORM_GET_FILE_TIME(OSXGetFileTime)
{
    s64 Result = 0;
    struct stat Stat;

    int Handle;
    if((Handle = open(FilePath, O_RDONLY)) != -1)
    {
        if(fstat(Handle, &Stat) != -1)
        {
            Result = Stat.st_mtimespec.tv_sec * 10000000 + Stat.st_mtimespec.tv_nsec / 100;
        }
    }
    close(Handle);
    return Result;
}


@interface LudusView : NSOpenGLView
{
   @public
    CVDisplayLinkRef displayLink;
    BOOL intialized;
    IOHIDManagerRef controllerManager;
}
@end


@implementation LudusView

void
controllerAdded(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device)
{
    CFStringRef manufacturer = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey));
    CFStringRef product      = (CFStringRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
    uint productId           = 0; // IOHIDDevice_GetProductID(device);

    NSLog(@"Device was detected: %@ %@ id(%x)", (NSString*)manufacturer, (NSString*)product, productId);
}
void
controllerRemoved(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef device)
{
}

void
controllerAction(void* inContext, IOReturn inResult, void* inSender, IOHIDValueRef value)
{
}

- (void)setupControllers
{
    controllerManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    NSArray* criteria = @[
        @{
           [NSString stringWithUTF8String:kIOHIDDeviceUsagePageKey] : [NSNumber numberWithInt:kHIDPage_GenericDesktop],
           [NSString stringWithUTF8String:kIOHIDDeviceUsageKey] : [NSNumber numberWithInt:kHIDUsage_GD_Joystick]
        },
        @{
           (NSString*)CFSTR(kIOHIDDeviceUsagePageKey) : [NSNumber numberWithInt:kHIDPage_GenericDesktop],
           (NSString*)CFSTR(kIOHIDDeviceUsageKey) : [NSNumber numberWithInt:kHIDUsage_GD_GamePad]
        },
        @{
           (NSString*)CFSTR(kIOHIDDeviceUsagePageKey) : [NSNumber numberWithInt:kHIDPage_GenericDesktop],
           (NSString*)CFSTR(kIOHIDDeviceUsageKey) : [NSNumber numberWithInt:kHIDUsage_GD_MultiAxisController]
        },
        @{
           (NSString*)CFSTR(kIOHIDDeviceUsagePageKey) : [NSNumber numberWithInt:kHIDPage_GenericDesktop],
           (NSString*)CFSTR(kIOHIDDeviceUsageKey) : [NSNumber numberWithInt:kHIDUsage_GD_Keyboard]
        }
    ];

    IOHIDManagerSetDeviceMatchingMultiple(controllerManager, (__bridge CFArrayRef)criteria);
    IOHIDManagerScheduleWithRunLoop(controllerManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(controllerManager, controllerAdded, (void*)self);
    IOHIDManagerRegisterDeviceRemovalCallback(controllerManager, controllerRemoved, (void*)self);
    IOHIDManagerOpen(controllerManager, kIOHIDOptionsTypeNone);
    IOHIDManagerRegisterInputValueCallback(controllerManager, controllerAction, (void*)self);
}

- (void)tickGameUpdateAndRender:(BOOL)updatingSize
{
    // we are setup and running
    if(GlobalRunning)
    {
        CGLLockContext([[self openGLContext] CGLContextObj]);

        [[self openGLContext] makeCurrentContext];

        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        game_input GameInput[2] = {};
        game_input* OldInput    = GameInput;
        game_input* NewInput    = GameInput + 1;
        game_input* TmpInput;

        RewindRenderBuffer(&RenderBuffer);
        if(GameCode.GameUpdateAndRender && !GlobalPause)
        {
            GameCode.GameUpdateAndRender(&GameMemory, NewInput, &RenderBuffer);
        }
        EndRenderBuffer(&RenderBuffer);

        OpenGLRenderBuffer(&RenderBuffer);

        [[self openGLContext] flushBuffer];

        CGLUnlockContext([[self openGLContext] CGLContextObj]);
    }
}

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime
{
#pragma unused(outputTime)

    @autoreleasepool
    {
        [self tickGameUpdateAndRender:NO];
    }

    return kCVReturnSuccess;
}

static CVReturn
GLXViewDisplayLinkCallback(CVDisplayLinkRef displayLink,
                           const CVTimeStamp* now,
                           const CVTimeStamp* outputTime,
                           CVOptionFlags inFlags,
                           CVOptionFlags* outFlags,
                           void* displayLinkContext)
{
    LudusView* view = (__bridge LudusView*)displayLinkContext;

    CVReturn result = [view getFrameForTime:outputTime];

    return result;
}

- (void)reshape
{
    [super reshape];

    NSRect rect = [self bounds];

    RenderBuffer.Width  = rect.size.width;
    RenderBuffer.Height = rect.size.height;
    OpenGLUpdateSize(rect.size.width, rect.size.height);
}

- (void)initialize
{
    if(intialized)
    {
        return;
    }

// we set the working directory to the resources folder within the bundle
#ifdef LUDUS_OS_BUNDLE
    NSFileManager* FileManager = [NSFileManager defaultManager];
    NSString* AppPath = [NSString stringWithFormat:@"%@/Contents/Resources", [[NSBundle mainBundle] bundlePath]];
    if([FileManager changeCurrentDirectoryPath:AppPath] == NO)
    {
        Assert(0);
    }
#endif

    GameMemory.PermanentStorageSize          = Megabytes(256);
    GameMemory.TransientStorageSize          = Gigabytes(1);
    GameMemory.PlatformAPI.AllocateMemory    = OSXPlatformAllocateMemory;
    GameMemory.PlatformAPI.DeallocateMemory  = OSXPlatformDeallocateMemory;
    GameMemory.PlatformAPI.FreeFileMemory    = OSXPlatformFreeFileMemory;
    GameMemory.PlatformAPI.ReadEntireFile    = OSXPlatfromReadEntireFile;
    GameMemory.PlatformAPI.WriteEntireFile   = OSXPlatformWriteEntireFile;
    GameMemory.PlatformAPI.Log               = OSXPlatformLog;
    GameMemory.PlatformAPI.OutputDebugString = OSXOutputDebugString;
    GameMemory.PlatformAPI.GetFileTime       = OSXGetFileTime;

    Platform = GameMemory.PlatformAPI;

    [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    NSOpenGLPixelFormatAttribute attrs[] = {NSOpenGLPFAOpenGLProfile,
                                            NSOpenGLProfileVersion3_2Core,
                                            NSOpenGLPFAColorSize,
                                            24,
                                            NSOpenGLPFAAlphaSize,
                                            8,
                                            NSOpenGLPFAAccelerated,
                                            NSOpenGLPFADoubleBuffer,
                                            0};

    NSOpenGLPixelFormatAttribute fallbackAttrs[] = {NSOpenGLPFADoubleBuffer,
                                                    NSOpenGLPFADepthSize,
                                                    32,
                                                    NSOpenGLPFAStencilSize,
                                                    (NSOpenGLPixelFormatAttribute)8,
                                                    (NSOpenGLPixelFormatAttribute)0};

    GLFallBack = false;

    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if(pf == nil)
    {
        OSX_LOG("Error creating OpenGLPixelFormat with accelerated attributes...trying fallback attributes\n");

        pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:fallbackAttrs];

        if(pf == nil)
        {
            printf("Error creating OpenGLPixelFormat with fallback attributes\n");
        }
        else
        {
            GLFallBack = true;
        }
    }

    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];

    [self setPixelFormat:pf];
    [self setOpenGLContext:context];

    [context makeCurrentContext];

    InitOpenGL(true);

#ifdef LUDUS_INTERNAL
    void* BaseAddress = (void*)Terabytes(2);
#else
    void* BaseAddress = 0;
#endif

    u64 MemTotalSize      = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    void* GameMemoryBlock = mmap(BaseAddress, MemTotalSize, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(GameMemoryBlock == MAP_FAILED)
    {
        OSX_LOG("FAILED");
    }

    GameMemory.PermanentStorage = GameMemoryBlock;
    GameMemory.TransientStorage = ((u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

    RenderBuffer.Size = Megabytes(4);
    RenderBuffer.Base = (uint8*)mmap(0, RenderBuffer.Size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);

    if(RenderBuffer.Base == MAP_FAILED)
    {
        OSX_LOG("FAILED");
    }

    [self setupControllers];


    if(GameMemoryBlock && GameMemory.PermanentStorage && GameMemory.TransientStorage && RenderBuffer.Base)
    {
        osx_state OSXState = {0};
        OSXGetEXEFileName(&OSXState);

        char SourceGameCodeDLLFullPath[4096];
        OSXBuildEXEPathFileName(
            &OSXState, (char*)"libgame.dylib", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

        GameCode = OsxLoadGameCode(SourceGameCodeDLLFullPath, "");
    }

    GlobalRunning = true;

    intialized = YES;
}

- (id)init
{
    self = [super init];
    if(self == nil)
    {
        return nil;
    }

    [self initialize];
    return self;
}

- (void)awakeFromNib
{
    [self initialize];
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];

    [[self openGLContext] makeCurrentContext];

    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

    CVReturn cvreturn = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);

    cvreturn = CVDisplayLinkSetOutputCallback(displayLink, &GLXViewDisplayLinkCallback, (__bridge void*)(self));

    CGLContextObj cglContext         = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    cvreturn = CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);

    CVDisplayLinkStart(displayLink);
}

- (void)dealloc
{
    [super dealloc];

    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    return YES;
}

- (BOOL)resignFirstResponder
{
    return YES;
}

@end

@interface LudusAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation LudusAppDelegate

- (void)applicationDidFinishLaunching:(id)sender
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    return YES;
}

- (void)applicationWillTerminate:(NSApplication*)sender
{
}

@end

int
main(int argc, const char* argv[])
{
    @autoreleasepool
    {
        NSApplication* app = [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        [app setDelegate:[[LudusAppDelegate alloc] init]];

        NSRect screenRect = [[NSScreen mainScreen] frame];
        float w           = WINDOW_WIDTH;
        float h           = WINDOW_HEIGHT;
        NSRect frame      = NSMakeRect((screenRect.size.width - w) * 0.5, (screenRect.size.height - h) * 0.5, w, h);

        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSTitledWindowMask | NSClosableWindowMask |
                                                                 NSMiniaturizableWindowMask | NSResizableWindowMask
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];

        LudusView* view = [[LudusView alloc] init];
        [view setFrame:[[window contentView] bounds]];
        [view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        [[window contentView] addSubview:view];
        [window setMinSize:NSMakeSize(100, 100)];
        [window setTitle:@WINDOW_NAME];
        [window makeKeyAndOrderFront:nil];

        [NSApp run];
    }
}
