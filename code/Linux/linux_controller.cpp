/*
    Creation Date: 04-04-2016
    Creator: Alex Baines
*/

#include <glob.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <sys/inotify.h>

#include "linux_controller_configs.h"

// struct definitions

typedef union
{
    struct
    {
        u8 IsHat : 1;
        u8 IsYAxis : 1;
        u8 HatState : 6;
        u8 PairIndex;
        u8 HatIndex;
    };
    struct
    {
        u8 _pad[3];
        u8 Index;
    };
} linux_controller_action;

typedef struct
{
    int Device;
    u8 NumAxes;
    u8 NumBtns;
    linux_controller_action* Actions;
    u32 HatData[4];
    int HatDataIndex;

} linux_controller;

typedef struct
{
    u16 Code;
    u16 OrigIndex;

} linux_controller_id;

enum
{
    HAT_MASK_UP    = 0x01,
    HAT_MASK_RIGHT = 0x02,
    HAT_MASK_DOWN  = 0x04,
    HAT_MASK_LEFT  = 0x08,

    HAT_MASK_UPRIGHT   = (HAT_MASK_UP | HAT_MASK_RIGHT),
    HAT_MASK_UPLEFT    = (HAT_MASK_UP | HAT_MASK_LEFT),
    HAT_MASK_DOWNRIGHT = (HAT_MASK_DOWN | HAT_MASK_RIGHT),
    HAT_MASK_DOWNLEFT  = (HAT_MASK_DOWN | HAT_MASK_LEFT),
};

enum
{
    HAT_ID_INVALID   = -1,
    HAT_ID_UPLEFT    = 0,
    HAT_ID_UP        = 1,
    HAT_ID_UPRIGHT   = 2,
    HAT_ID_RIGHT     = 3,
    HAT_ID_DOWNRIGHT = 4,
    HAT_ID_DOWN      = 5,
    HAT_ID_DOWNLEFT  = 6,
    HAT_ID_LEFT      = 7,
};

// globals

global_variable linux_controller LinuxControllers[NUM_CONTROLLERS];
global_variable int INotifyControllerHandle;
global_variable int INotifyControllerWatch;

#define SPAMMY_CONTROLLER_OUTPUT

#if defined(LUDUS_INTERNAL) && defined(SPAMMY_CONTROLLER_OUTPUT)
#define LINUX_CONTROLLER_DEBUG(fmt, ...) LinuxLogF(fmt, ##__VA_ARGS__);
#else
#define LINUX_CONTROLLER_DEBUG(fmt, ...)
#endif

#define NUM_BUTTONS 10
#define NUM_AXES 4

// NOTE(inso): Index 0 of ButtonNames maps to game_input->Button[4], since the first 4 are handled by the sticks?
// Also the Actions array uses 1 to refer to index 0 here, since 0 is reserved for "unused" buttons.
// this is why [Button - 1] and [Button + 3] are used in the HandleEvent function below

global_variable const char* ButtonNames[] = {"dpup",
                                             "dpdown",
                                             "dpleft",
                                             "dpright",
                                             "a",
                                             "b",
                                             "x",
                                             "y",
                                             "start",
                                             "back",

                                             "leftx",
                                             "lefty",
                                             "rightx",
                                             "righty",

                                             NULL};

// static func forward declarations

static void
LinuxControllerGetGUID(const char* Dev, char* GUID);

static u8
LinuxControllerGetHatMask(s16 Value, b32 IsYAxis);

static int
LinuxControllerHatMaskToID(u8 State);

static u8
LinuxControllerGetHatButton(linux_controller* Controller, int IA, int IB);

static int
LinuxControllerIDCompare(const void* A, const void* B);

static void
LinuxControllerLogMapping(linux_controller* C);

// extern funcs

void
LinuxControllerAdd(game_controller_input* Input, const char* AddPath, struct stat* AddStat)
{
    int NextFreeSlot = -1;
    b32 ShouldAdd    = 1;

    for(int i = 0; i < NUM_CONTROLLERS; ++i)
    {
        if(!Input[i].IsConnected)
        {
            if(NextFreeSlot == -1)
                NextFreeSlot = i;
            continue;
        }

        if(AddStat)
        {
            struct stat CurStat;
            if(fstat(LinuxControllers[i].Device, &CurStat) != -1 && CurStat.st_ino == AddStat->st_ino)
            {
                ShouldAdd = 0;
                break;
            }
        }
    }

    int JSDev;
    if(NextFreeSlot == -1 || !ShouldAdd || (JSDev = open(AddPath, O_RDONLY | O_NONBLOCK)) == -1)
    {
        return;
    }

    linux_controller* C = LinuxControllers + NextFreeSlot;

    C->Device                       = JSDev;
    Input[NextFreeSlot].IsConnected = 1;
    Input[NextFreeSlot].IsAnalog    = 1; // FIXME

    char Name[256] = {};
    ioctl(JSDev, JSIOCGNAME(sizeof(Name)), Name);

    char GUID[33] = {};
    LinuxControllerGetGUID(basename(AddPath), GUID);

    // TODO: check environment variable first
    const char* Mapping = NULL;
    for(const char** P = ControllerConfigs; *P; ++P)
    {
        if(strncmp(GUID, *P, 32) == 0)
        {
            Mapping = *P;
            break;
        }
    }

    ioctl(JSDev, JSIOCGAXES, &C->NumAxes);
    ioctl(JSDev, JSIOCGBUTTONS, &C->NumBtns);

    LinuxLogF(
        "Opened controller %d: [%s], axes: %d, btns: %d GUID: [%s]", NextFreeSlot, Name, C->NumAxes, C->NumBtns, GUID);

    C->Actions = (linux_controller_action*)calloc(C->NumBtns + C->NumAxes, sizeof(*C->Actions));

    if(!Mapping)
    {
        u8 DefaultBtnMap[]  = {5, 6, 7, 8, 1, 2, 3, 4, 9, 10};
        u8 DefaultAxisMap[] = {11, 12, 13, 14};

        LinuxLogF("Warning: no mapping for controller. Will try fallback.");

        // TODO: Better default mapping?

        for(int i = 0; i < ArrayCount(DefaultBtnMap); ++i)
        {
            if(i >= C->NumBtns)
                break;
            C->Actions[i].Index = DefaultBtnMap[i];
        }

        for(int i = 0; i < ArrayCount(DefaultAxisMap); ++i)
        {
            if(i >= C->NumAxes)
                break;
            C->Actions[C->NumBtns + i].Index = DefaultAxisMap[i];
        }

        LinuxControllerLogMapping(C);

        return;
    }
    else
    {
        LinuxLogF("Got Mapping.");
    }

    uint8_t AxisIDs[ABS_MAX + 1]            = {};
    uint16_t BtnIDs[KEY_MAX - BTN_MISC + 1] = {};

    ioctl(JSDev, JSIOCGAXMAP, AxisIDs);
    ioctl(JSDev, JSIOCGBTNMAP, BtnIDs);

    linux_controller_id* SortedAxisIDs = (linux_controller_id*)alloca(C->NumAxes * sizeof(*SortedAxisIDs));
    for(int i = 0; i < C->NumAxes; ++i)
    {
        SortedAxisIDs[i].Code      = AxisIDs[i];
        SortedAxisIDs[i].OrigIndex = i;
    }
    qsort(SortedAxisIDs, C->NumAxes, sizeof(*SortedAxisIDs), &LinuxControllerIDCompare);

    linux_controller_id* SortedBtnIDs = (linux_controller_id*)alloca(C->NumBtns * sizeof(*SortedBtnIDs));
    for(int i = 0; i < C->NumBtns; ++i)
    {
        SortedBtnIDs[i].Code      = BtnIDs[i];
        SortedBtnIDs[i].OrigIndex = i;
    }
    qsort(SortedBtnIDs, C->NumBtns, sizeof(*SortedBtnIDs), &LinuxControllerIDCompare);

    const char *P, *Sep;
    for(const char* PrevP = Mapping; *PrevP; PrevP = P)
    {
        ++PrevP;
        P = strchrnul(PrevP, ',');

        if((Sep = (const char*)memchr(PrevP, ':', P - PrevP)))
        {
            LinuxLogF("Mapping [%.*s]", (int)(P - PrevP), PrevP);

            int Index = 0;
            int HatID = 0;

            enum
            {
                MAPTYPE_INVALID,
                MAPTYPE_BUTTON,
                MAPTYPE_AXIS,
                MAPTYPE_HAT
            } Type = MAPTYPE_INVALID;

            switch(Sep[1])
            {
                case 'a':
                {
                    Type  = MAPTYPE_AXIS;
                    Index = strtoul(Sep + 2, NULL, 10);
                    LINUX_CONTROLLER_DEBUG("  Got Axis %d", Index);
                }
                break;

                case 'b':
                {
                    Type  = MAPTYPE_BUTTON;
                    Index = strtoul(Sep + 2, NULL, 10);
                    LINUX_CONTROLLER_DEBUG("  Got Button %d", Index);
                }
                break;

                case 'h':
                {
                    LINUX_CONTROLLER_DEBUG("  Going to parse hat... wish me luck");
                    unsigned int HatMask;
                    sscanf(Sep + 2, "%u.%1u", &Index, &HatMask);

                    if(!HatMask)
                    {
                        LINUX_CONTROLLER_DEBUG("  Hat mask was 0?");
                        break;
                    }

                    HatID = LinuxControllerHatMaskToID(HatMask);

                    int HatIndexX = -1, HatIndexY = -1;

                    for(int i = 0; i < C->NumAxes; ++i)
                    {
                        if(SortedAxisIDs[i].Code == (ABS_HAT0X + (Index * 2)))
                            HatIndexX = SortedAxisIDs[i].OrigIndex;
                        if(SortedAxisIDs[i].Code == (ABS_HAT0Y + (Index * 2)))
                            HatIndexY = SortedAxisIDs[i].OrigIndex;
                    }

                    LINUX_CONTROLLER_DEBUG("  Hat indices: %d %d", HatIndexX, HatIndexY);

                    if(HatIndexX >= 0 && HatIndexY >= 0 && (HatIndexX != HatIndexY))
                    {
                        linux_controller_action *X = C->Actions + HatIndexX, *Y = C->Actions + HatIndexY;

                        Type = MAPTYPE_HAT;

                        X->IsHat     = 1;
                        X->IsYAxis   = 0;
                        X->HatState  = 0;
                        X->PairIndex = HatIndexY;
                        X->HatIndex  = C->HatDataIndex;

                        Y->IsHat     = 1;
                        Y->IsYAxis   = 1;
                        Y->HatState  = 0;
                        Y->PairIndex = HatIndexX;
                        Y->HatIndex  = C->HatDataIndex;

                        LINUX_CONTROLLER_DEBUG("  Got Hat %d %d %d", Index, HatMask, HatID);
                    }
                    else
                    {
                        LINUX_CONTROLLER_DEBUG(
                            "Binding to a hat, but the controller doesn't seem to have any... Wrong mapping?");
                    }
                }
                break;
            };

            if(Type == MAPTYPE_INVALID)
            {
                LINUX_CONTROLLER_DEBUG("invalid map type wtf");
                continue;
            }
            else if(Type == MAPTYPE_BUTTON)
            {
                if(Index >= C->NumBtns)
                {
                    LINUX_CONTROLLER_DEBUG("!! Can't bind to b%d, only %d buttons.", Index, C->NumBtns);
                    continue;
                }
                Index = SortedBtnIDs[Index].OrigIndex;
            }
            else if(Type == MAPTYPE_AXIS)
            {
                if(Index >= C->NumAxes)
                {
                    LINUX_CONTROLLER_DEBUG("!! Can't bind to a%d, only %d axes.", Index, C->NumAxes);
                    continue;
                }
                Index = SortedAxisIDs[Index].OrigIndex;
            }

            LINUX_CONTROLLER_DEBUG("  New index: %d", Index);

            for(const char** Btn = ButtonNames; *Btn; ++Btn)
            {
                if(strncmp(*Btn, PrevP, Sep - PrevP) == 0 && (*Btn)[Sep - PrevP] == 0)
                {
                    uint8_t BtnIndex = 1 + (Btn - ButtonNames);
                    LINUX_CONTROLLER_DEBUG("  Found [%s] -> %d", *Btn, BtnIndex);

                    if(Type == MAPTYPE_HAT)
                    {
                        C->HatData[C->HatDataIndex++] |= (BtnIndex << (HatID * 4));
                    }
                    else if(Type == MAPTYPE_BUTTON)
                    {
                        C->Actions[Index].Index = BtnIndex;
                    }
                    else if(Type == MAPTYPE_AXIS)
                    {
                        C->Actions[Index + C->NumBtns].Index = BtnIndex;
                    }
                    break;
                }
            }
        }
    }

    LinuxControllerLogMapping(C);
}

void
LinuxControllerInit(game_controller_input* Input)
{
    INotifyControllerHandle = inotify_init();
    INotifyControllerWatch  = inotify_add_watch(INotifyControllerHandle, "/dev/input", IN_CREATE | IN_ATTRIB);

    int Flags = fcntl(INotifyControllerHandle, F_GETFL);
    fcntl(INotifyControllerHandle, F_SETFL, Flags | O_NONBLOCK);

    glob_t GlobData;
    if(glob("/dev/input/js*", 0, 0, &GlobData) == 0)
    {
        for(int i = 0; i < GlobData.gl_pathc; ++i)
        {
            LinuxControllerAdd(Input, GlobData.gl_pathv[i], 0);
        }
        globfree(&GlobData);
    }
}

void
LinuxControllerHandleEvent(game_controller_input* Input, int Index, struct js_event* Event)
{
    game_controller_input* State = Input + Index;
    linux_controller* Controller = LinuxControllers + Index;

    r32* Axes[] = {
        &State->LeftStickAverageX, &State->LeftStickAverageY, &State->RightStickAverageX, &State->RightStickAverageY};

    u8 Button;

    switch(Event->type)
    {
        case JS_EVENT_BUTTON:
        {
            if((Button = Controller->Actions[Event->number].Index))
            {
                Input->Buttons[Button + 3].EndedDown = Event->value;
                if(Event->value)
                {
                    Input->Buttons[Button + 3].HalfTransitionCount++;
                }
                LINUX_CONTROLLER_DEBUG(
                    "[C%d] %s %s.", Index, ButtonNames[Button - 1], Event->value ? "pressed" : "released");
            }
        }
        break;

        case JS_EVENT_AXIS:
        {
            linux_controller_action* Action = Controller->Actions + Controller->NumBtns + Event->number;

            if(Action->IsHat)
            {
                linux_controller_action* Pair = Controller->Actions + Controller->NumBtns + Action->PairIndex;

                // TODO: need to improve this a bit so that e.g. upright triggers all 3 bindings
                // on upright, up, right. not just upright.

                int IA = Action - Controller->Actions, IB = Pair - Controller->Actions;

                if((Button = LinuxControllerGetHatButton(Controller, IA, IB)))
                {
                    Input->Buttons[Button + 3].EndedDown = 0;
                    LINUX_CONTROLLER_DEBUG("[C%d] %s released [via hat]", Index, ButtonNames[Button - 1]);
                }

                Action->HatState |= LinuxControllerGetHatMask(Event->value, Action->IsYAxis);

                if((Button = LinuxControllerGetHatButton(Controller, IA, IB)))
                {
                    Input->Buttons[Button + 3].EndedDown = 1;
                    Input->Buttons[Button + 3].HalfTransitionCount++;
                    LINUX_CONTROLLER_DEBUG("[C%d] %s pressed [via hat]", Index, ButtonNames[Button - 1]);
                }
            }
            else
            {
                int AxisIndex = Action->Index - (NUM_BUTTONS + 1);

                if(AxisIndex >= 0 && AxisIndex < NUM_AXES)
                {
                    r32 Val = Event->value / 32767.0f;
                    if(Val < -1.0f)
                        Val = -1.0f;

                    *Axes[AxisIndex] = Val;

                    LINUX_CONTROLLER_DEBUG("[C%d] %s: %.2f", Index, ButtonNames[Action->Index - 1], Val);
                }
            }
        }
        break;
    }
}

void
LinuxControllerUpdateAll(game_controller_input* Input)
{
    for(int i = 0; i < ArrayCount(LinuxControllers); ++i)
    {
        if(!Input[i].IsConnected)
            continue;

        for(int j = 0; j < ArrayCount(Input[i].Buttons); ++j)
        {
            Input[i].Buttons[j].HalfTransitionCount = 0;
        }

        struct js_event e = {};
        ssize_t result;

        while((result = read(LinuxControllers[i].Device, &e, sizeof(e))) > 0)
        {
            e.type &= ~JS_EVENT_INIT;
            LinuxControllerHandleEvent(Input, i, &e);
        }

        if(result == -1)
        {
            if(errno == ENODEV)
            {
                LinuxLogF("Controller %d disconnected.", i);

                close(LinuxControllers[i].Device);
                free(LinuxControllers[i].Actions);

                LinuxControllers[i] = {};

                Input[i].IsConnected = 0;
            }
            else if(errno != EAGAIN)
            {
                LINUX_FN_LOG("read: %s", strerror(errno));
            }
        }
    }

    char Path[PATH_MAX] = "/dev/input/";
    struct inotify_event* INEvent;
    char Buffer[sizeof(*INEvent) + NAME_MAX + 1];

    ssize_t ReadSize = read(INotifyControllerHandle, &Buffer, sizeof(Buffer));
    for(char* Ptr = Buffer; (Ptr - Buffer) < ReadSize; Ptr += (sizeof(*INEvent) + INEvent->len))
    {
        INEvent = (struct inotify_event*)Ptr;

        if(INEvent->wd != INotifyControllerWatch || !(INEvent->mask & (IN_CREATE | IN_ATTRIB)))
        {
            continue;
        }

        struct stat StatData = {};

        if(strncmp(INEvent->name, "js", 2) == 0 && INEvent->len < (PATH_MAX - 11))
        {
            memcpy(Path + 11, INEvent->name, INEvent->len);
            if(stat(Path, &StatData) == 0)
            {
                LinuxControllerAdd(Input, Path, &StatData);
            }
        }
    }
}

// static func definitions

static void
LinuxControllerGetGUID(const char* Device, char* GUID)
{
    FILE* F = fopen("/proc/bus/input/devices", "r");

    char Line[256];
    u16 GUID_U16[8] = {};

    size_t DevLen = strlen(Device);
    char* Dev     = (char*)alloca(DevLen + 2);
    memcpy(Dev, Device, DevLen);
    Dev[DevLen]     = ' ';
    Dev[DevLen + 1] = 0;

    int GotGuid = 0;

    while(fgets(Line, sizeof(Line), F))
    {
        if(*Line == '\n')
        {
            memset(GUID_U16, 0, sizeof(GUID_U16));
        }
        else if(*Line == 'H' && strstr(Line, Dev))
        {
            GotGuid = 1;
            break;
        }
        else
        {
            sscanf(Line,
                   "I: Bus=%4hx Vendor=%4hx Product=%4hx Version=%4hx",
                   GUID_U16,
                   GUID_U16 + 2,
                   GUID_U16 + 4,
                   GUID_U16 + 6);
        }
    }

    fclose(F);

    if(!GotGuid)
    {
        memset(GUID_U16, 0, sizeof(GUID_U16));
    }
    else
    {
        GotGuid = 0;
        for(int i = 2; i < 8; ++i)
        {
            if(GUID_U16[i] != 0)
            {
                GotGuid = 1;
                break;
            }
        }
    }

    if(!GotGuid)
    {
        // TODO: copy the name as the guid or some shit that sdl does
    }

    for(int i = 0; i < 8; ++i)
    {
        snprintf(GUID + (i * 4), 33 - (i * 4), "%02x%02x", GUID_U16[i] & 0xFF, GUID_U16[i] >> 8);
    }
}

static u8
LinuxControllerGetHatMask(s16 Val, b32 IsYAxis)
{
    uint8_t Mask = 0;

    if(Val > 0)
    {
        Mask = IsYAxis ? HAT_MASK_UP : HAT_MASK_RIGHT;
    }
    else if(Val < 0)
    {
        Mask = IsYAxis ? HAT_MASK_DOWN : HAT_MASK_LEFT;
    }

    return Mask;
}

static int
LinuxControllerHatMaskToID(u8 State)
{
    switch(State)
    {
        case HAT_MASK_UP:
            return HAT_ID_UP;
        case HAT_MASK_RIGHT:
            return HAT_ID_RIGHT;
        case HAT_MASK_DOWN:
            return HAT_ID_DOWN;
        case HAT_MASK_LEFT:
            return HAT_ID_LEFT;

        case HAT_MASK_UPRIGHT:
            return HAT_ID_UPRIGHT;
        case HAT_MASK_UPLEFT:
            return HAT_ID_UPLEFT;
        case HAT_MASK_DOWNRIGHT:
            return HAT_ID_DOWNRIGHT;
        case HAT_MASK_DOWNLEFT:
            return HAT_ID_DOWNLEFT;

        default:
            return HAT_ID_INVALID;
    }
}

static uint8_t
LinuxControllerGetHatButton(linux_controller* Controller, int IA, int IB)
{
    linux_controller_action* A = Controller->Actions + IA;
    linux_controller_action* B = Controller->Actions + IB;

    //    assert(A->HatIndex == B->HatIndex);
    u8 Index = A->HatIndex;

    u32 ButtonBitset = Controller->HatData[Index];
    int HatID        = LinuxControllerHatMaskToID(A->HatState | B->HatState);

    if(HatID == HAT_ID_INVALID)
    {
        return 0;
    }

    return (ButtonBitset >> (HatID * 4)) & 0xF;
}

static int
LinuxControllerIDCompare(const void* A, const void* B)
{
    return ((linux_controller_id*)A)->Code - ((linux_controller_id*)B)->Code;
}

static void
LinuxControllerLogMapping(linux_controller* C)
{
    LinuxLogF("BUTTONS:");
    for(int i = 0; i < C->NumBtns; ++i)
    {
        int Idx = C->Actions[i].Index;
        LinuxLogF(" %d: %d (%s)", i, Idx, Idx == 0 ? "UNUSED" : ButtonNames[Idx - 1]);
    }

    LinuxLogF("AXES:");
    for(int i = C->NumBtns; i < C->NumBtns + C->NumAxes; ++i)
    {
        int Idx = C->Actions[i].Index;
        LinuxLogF(" %d: %d (%s)", i, Idx, Idx == 0 ? "UNUSED" : ButtonNames[Idx - 1]);
    }
}
