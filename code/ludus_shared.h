/*
    Creation Date: 29-03-2016
    Creator: Emil Lauridsen
*/

#if !defined(LUDUS_SHARED_H)
#include "ludus_platform.h"

typedef struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;
} memory_arena;

void
InitializeArena(memory_arena* Arena, memory_index Size, void* Base);
void
EmptyArena(memory_arena* Arena);
uint8*
PushSize(memory_arena* Arena, memory_index Size);

#define PushStruct(Arena, Type) (Type*)PushSize(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type*)PushSize(Arena, Count * sizeof(Type))

/* Example usage of temporary sections:
>    StartTemporarySection(Arena);
>    // _Every_ allocation after this point is temporary
>    queue* Queue = MakeQueue(Arena, 9001);
>    // Do stuff with queue..
>    EndTemporarySection(Arean);
>    // All allocations since the matching StartTemporarySection are now freed
*/
#define StartTemporarySection(Arena)                                                                                   \
    {                                                                                                                  \
        memory_index __Used = Arena->Used;
#define EndTemporarySection(Arena)                                                                                     \
    Arena->Used = __Used;                                                                                              \
    }

typedef struct queue
{
    memory_index Size;
    void** Data;
    s64 Back;
    s64 Front;
} queue;

queue
MakeQueue(memory_arena* Arena, memory_index Size);

void
Enqueue(queue* Queue, void* Data);

void*
Dequeue(queue* Queue);

#define LUDUS_SHARED_H
#endif
