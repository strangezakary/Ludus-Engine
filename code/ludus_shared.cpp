/*
    Creation Date: 29-03-2016
    Creator: Emil Lauridsen
*/

#include "ludus_shared.h"

void
InitializeArena(memory_arena* Arena, memory_index Size, void* Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8*)Base;
    Arena->Used = 0;
};

void
EmptyArena(memory_arena* Arena)
{
    Arena->Used = 0;
}

uint8*
PushSize(memory_arena* Arena, memory_index Size)
{
    Assert(Arena->Used + Size <= Arena->Size);

    uint8* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}

queue
MakeQueue(memory_arena* Arena, memory_index Size)
{
    queue Result = {0};
    Result.Size  = Size;
    Result.Data  = PushArray(Arena, void*, Size);
    Result.Back  = -1;
    return (Result);
}

void
Enqueue(queue* Queue, void* Data)
{
    if(Queue->Front == Queue->Back)
    {
        Assert(!"Queue is full");
    }
    Queue->Data[Queue->Front] = Data;
    Queue->Front              = (Queue->Front + 1) % Queue->Size;
}

void*
Dequeue(queue* Queue)
{
    void* Result = 0;
    if(Queue->Back + 1 != Queue->Front)
    {
        Queue->Back = (Queue->Back + 1) % Queue->Size;
        Result      = Queue->Data[Queue->Back];
    }
    return (Result);
}
