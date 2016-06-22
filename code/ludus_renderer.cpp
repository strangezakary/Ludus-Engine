/*
    Creation Date: 29-03-2016
    Creator: Emil Lauridsen
*/

#include "ludus_renderer.h"

internal texture_slot*
AllocateTextureSlot(render_buffer* RenderBuffer)
{
    texture_slot* Result = 0;
    if(RenderBuffer->FreeTextureSlot)
    {
        Result                        = RenderBuffer->FreeTextureSlot;
        RenderBuffer->FreeTextureSlot = Result->NextFree;
        Result->NextFree              = 0;
    }
    else
    {
        if(RenderBuffer->TextureCount < RENDERER_TEXTURE_SLOTS - 1)
        {
            ++RenderBuffer->TextureCount;
            Result = &RenderBuffer->TextureSlots[RenderBuffer->TextureCount];
        }
        else
        {
            Assert(!"No more available texture slots");
        }
    }
    return (Result);
}

internal void
FreeTextureSlot(render_buffer* RenderBuffer, texture_slot* Slot)
{
    Slot->NextFree                = RenderBuffer->FreeTextureSlot;
    RenderBuffer->FreeTextureSlot = Slot;
}

internal font_slot*
AllocateFontSlot(render_buffer* RenderBuffer)
{
    font_slot* Result = 0;
    if(RenderBuffer->FreeFontSlot)
    {
        Result                     = RenderBuffer->FreeFontSlot;
        RenderBuffer->FreeFontSlot = Result->NextFree;
        Result->NextFree           = 0;
    }
    else
    {
        if(RenderBuffer->FontCount < RENDERER_TEXTURE_SLOTS - 1)
        {
            ++RenderBuffer->FontCount;
            Result = &RenderBuffer->FontSlots[RenderBuffer->FontCount];
        }
        else
        {
            Assert(!"No more available font slots");
        }
    }
    return (Result);
}

internal void
FreeFontSlot(render_buffer* RenderBuffer, font_slot* Slot)
{
    Slot->NextFree             = RenderBuffer->FreeFontSlot;
    RenderBuffer->FreeFontSlot = Slot;
}

#define CommandPushCase(Kind, Type)                                                                                    \
    case Kind:                                                                                                         \
    {                                                                                                                  \
        CopyMemory(RenderBuffer->Current, (u8*)Data, sizeof(Type));                                                    \
        RenderBuffer->Current += sizeof(Type);                                                                         \
    }                                                                                                                  \
    break;

internal void*
PushCommand(render_buffer* RenderBuffer, rc_kind Kind, void* Data)
{
    rc_header* Header = (rc_header*)RenderBuffer->Current;
    RenderBuffer->Current += sizeof(rc_header);
    void* Result = RenderBuffer->Current;

    Header->Kind = Kind;

    switch(Kind)
    {
        ForEachKind(CommandPushCase);

        case RenderCommand_DrawString:
        {
            CopyMemory(RenderBuffer->Current, (u8*)Data, sizeof(rc_draw_string));
            RenderBuffer->Current += sizeof(rc_draw_string);
        }
        break;

        default:
        {
            Assert(!"Unhandled render command");
        }
        break;
    }

    return (Result);
}

internal rc_kind
GetNextKind(render_buffer* RenderBuffer)
{
    rc_header* Header = (rc_header*)RenderBuffer->Current;
    RenderBuffer->Current += sizeof(rc_header);
    return Header->Kind;
}

#define CommandGetCase(Kind, Type)                                                                                     \
    case Kind:                                                                                                         \
    {                                                                                                                  \
        RenderBuffer->Current += sizeof(Type);                                                                         \
    }                                                                                                                  \
    break;

internal void*
GetNextCommand(render_buffer* RenderBuffer, rc_kind Kind)
{
    void* Result = RenderBuffer->Current;

    switch(Kind)
    {
        ForEachKind(CommandGetCase);

        case RenderCommand_DrawString:
        {
            rc_draw_string* Command = (rc_draw_string*)RenderBuffer->Current;
            RenderBuffer->Current += sizeof(rc_draw_string) + Command->CharCount;
        }
        break;

        default:
        {
            Assert(!"Unhandled render command");
        }
        break;
    };

    return Result;
}

internal rc_draw_rect*
PushDrawRect(render_buffer* RenderBuffer, vec2 Mid, vec2 Size, texture_slot* TextureSlot)
{
    rc_draw_rect Command = {0};
    Command.Mid          = Mid;
    Command.Size         = Size;
    Command.Color        = Vec4i(1, 1, 1, 1);
    Command.TextureSlot  = TextureSlot;
    Command.MinUV        = Vec2i(0, 0);
    Command.MaxUV        = Vec2i(1, 1);

    return (rc_draw_rect*)PushCommand(RenderBuffer, RenderCommand_DrawRect, &Command);
}

internal rc_draw_rect_outline*
PushDrawRectOutline(render_buffer* RenderBuffer, vec2 Mid, vec2 Size, vec4 Color)
{
    rc_draw_rect_outline Command = {0};
    Command.Mid                  = Mid;
    Command.Size                 = Size;
    Command.Color                = Color;

    return (rc_draw_rect_outline*)PushCommand(RenderBuffer, RenderCommand_DrawRectOutline, &Command);
}

internal rc_draw_rect_multitextured*
PushDrawRectMultitextured(render_buffer* RenderBuffer, vec2 Mid, vec2 Size)
{
    rc_draw_rect_multitextured Command = {0};
    Command.Mid                        = Mid;
    Command.Size                       = Size;
    Command.Color                      = Vec4i(1, 1, 1, 1);

    return (rc_draw_rect_multitextured*)PushCommand(RenderBuffer, RenderCommand_DrawRectMultitextured, &Command);
}

internal rc_draw_line*
PushDrawLine(render_buffer* RenderBuffer, vec2 Start, vec2 End, vec4 Color)
{
    rc_draw_line Command = {0};
    Command.Start        = Start;
    Command.End          = End;
    Command.Color        = Color;

    return (rc_draw_line*)PushCommand(RenderBuffer, RenderCommand_DrawLine, &Command);
}

internal rc_load_texture*
PushLoadTexture(render_buffer* RenderBuffer, texture_slot* TextureSlot, bool32 Interpolate, char* Filename)
{
    rc_load_texture Command = {0};
    Command.TextureSlot     = TextureSlot;
    Command.Filename        = Filename;

    return (rc_load_texture*)PushCommand(RenderBuffer, RenderCommand_LoadTexture, &Command);
}

internal rc_load_texture_raw*
PushLoadTextureRaw(render_buffer* RenderBuffer,
                   texture_slot* TextureSlot,
                   bool32 Interpolate,
                   uint8* Data,
                   uint32 Width,
                   uint32 Height)
{
    rc_load_texture_raw Command = {0};
    Command.TextureSlot         = TextureSlot;
    Command.Data                = Data;
    Command.Width               = Width;
    Command.Height              = Height;

    return (rc_load_texture_raw*)PushCommand(RenderBuffer, RenderCommand_LoadTextureRaw, &Command);
}

internal rc_load_font*
PushLoadFont(render_buffer* RenderBuffer, font_slot* FontSlot, char* Filename, float Height)
{
    rc_load_font Command = {0};
    Command.FontSlot     = FontSlot;
    Command.Filename     = Filename;
    Command.Height       = Height;
    Command.OversampleX  = 2;
    Command.OversampleY  = 2;

    return (rc_load_font*)PushCommand(RenderBuffer, RenderCommand_LoadFont, &Command);
}

internal rc_draw_string*
PushDrawString(render_buffer* RenderBuffer, font_slot* FontSlot, vec2 Pos, float Scale, char* String)
{
    rc_draw_string Command = {0};
    Command.FontSlot       = FontSlot;
    Command.Pos            = Pos;
    Command.Color          = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    Command.Scale          = Scale;

    rc_draw_string* Result = (rc_draw_string*)PushCommand(RenderBuffer, RenderCommand_DrawString, &Command);
    while(*String)
    {
        *RenderBuffer->Current = (u8)*String;
        ++RenderBuffer->Current;
        ++Result->CharCount;
    }
    return (Result);
}

internal rc_draw_string*
PushDrawStringSized(
    render_buffer* RenderBuffer, font_slot* FontSlot, vec2 Pos, float Scale, char* String, uint16 Length)
{
    rc_draw_string Command = {0};
    Command.FontSlot       = FontSlot;
    Command.Pos            = Pos;
    Command.Color          = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    Command.Scale          = Scale;

    rc_draw_string* Result = (rc_draw_string*)PushCommand(RenderBuffer, RenderCommand_DrawString, &Command);
    for(uint16 i = 0; i < Length; ++i)
    {
        *RenderBuffer->Current = (u8)String[i];
        ++RenderBuffer->Current;
    }
    Result->CharCount = Length;
    return (Result);
}

internal rc_set_camera_matrix*
PushSetCameraMatrix(render_buffer* RenderBuffer, mat4 Matrix)
{
    rc_set_camera_matrix Command = {0};
    Command.Matrix               = Matrix;

    return (rc_set_camera_matrix*)PushCommand(RenderBuffer, RenderCommand_SetCameraMatrix, &Command);
}
