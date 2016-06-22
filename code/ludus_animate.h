/*
    Creation Date: 16-04-2016
    Creator: Emil Lauridsen
*/

#if !defined(LUDUS_ANIMATE_H)

typedef struct anim_state
{
    texture_slot* Sheet;
    uint16 FrameCount;
    uint16 Rows;
    uint16 Row;
    float FrameDuration;
    bool32 Repeat;

    uint16 CurrentFrame;
    float CurrentSubtime;
} anim_state;

#define LUDUS_ANIMATE_H
#endif
