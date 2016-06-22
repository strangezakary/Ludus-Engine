/*
    Creation Date: 16-04-2016
    Creator: Emil Lauridsen
*/

#include "ludus_animate.h"
#include "ludus_renderer.h"

anim_state
MakeAnimation(texture_slot* Sheet, uint16 Rows, uint16 Row, uint16 FrameCount, float FrameDuration, bool32 Repeat)
{
    anim_state Result    = {0};
    Result.Sheet         = Sheet;
    Result.FrameCount    = FrameCount;
    Result.Rows          = Rows;
    Result.Row           = Row;
    Result.FrameDuration = FrameDuration;
    Result.Repeat        = Repeat;
    return (Result);
}

void
StartAnimation(anim_state* State)
{
    State->CurrentFrame   = 0;
    State->CurrentSubtime = 0.0f;
}

void
UpdateAnimation(anim_state* State, float DeltaTime)
{
    State->CurrentSubtime += DeltaTime;
    if(State->CurrentSubtime > State->FrameDuration)
    {
        ++State->CurrentFrame;
        State->CurrentSubtime -= State->FrameDuration;
    }

    if(State->CurrentFrame >= State->FrameCount)
    {
        State->CurrentFrame = State->Repeat ? 0 : State->FrameCount - 1;
    }
}

rc_draw_rect*
RenderAnimation(anim_state* State, render_buffer* RenderBuffer, vec2 Mid, vec2 Size)
{
    float FrameStep = 1.0f / State->FrameCount;
    float RowHeight = 1.0f / State->Rows;
    vec2 MinUV      = Vec2((float)State->CurrentFrame * FrameStep, (float)(State->Row) * RowHeight);
    vec2 MaxUV      = Vec2((float)(State->CurrentFrame + 1) * FrameStep, (float)(State->Row + 1) * RowHeight);

    rc_draw_rect* Rect = PushDrawRect(RenderBuffer, Mid, Size, State->Sheet);
    Rect->MinUV        = MinUV;
    Rect->MaxUV        = MaxUV;
    return (Rect);
}
