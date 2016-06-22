/*
    Creation Date: 28-03-2016
    Creator: Emil Lauridsen
*/

#if !defined(LUDUS_RENDERER_H)

typedef struct texture_slot
{
    struct texture_slot* NextFree;
    uint32 Width;
    uint32 Height;
    uint8 ImplData[64];
} texture_slot;

typedef struct font_slot
{
    struct font_slot* NextFree;
    uint8 ImplData[4096];
} font_slot;

#define RENDERER_TEXTURE_SLOTS 1024
#define RENDERER_FONT_SLOTS 8

typedef struct render_buffer
{
    memory_index Size;
    uint8* Base;
    uint8* Current;
    uint8* End;

    uint32 TextureCount;
    texture_slot TextureSlots[RENDERER_TEXTURE_SLOTS];
    texture_slot* FreeTextureSlot;

    uint32 FontCount;
    font_slot FontSlots[RENDERER_FONT_SLOTS];
    font_slot* FreeFontSlot;

    uint32 Width;
    uint32 Height;
    vec4 ClearColor;
} render_buffer;

typedef enum rc_kind {
    RenderCommand_DrawRect,
    RenderCommand_DrawRectOutline,
    RenderCommand_DrawRectMultitextured,
    RenderCommand_DrawLine,
    RenderCommand_LoadTexture,
    RenderCommand_LoadTextureRaw,
    RenderCommand_LoadFont,
    RenderCommand_DrawString,
    RenderCommand_SetCameraMatrix,

    RenderCommandKindCount,
} rc_kind;

#define ForEachKind(Action)                                                                                            \
    Action(RenderCommand_DrawRect, rc_draw_rect);                                                                      \
    Action(RenderCommand_DrawRectOutline, rc_draw_rect_outline);                                                       \
    Action(RenderCommand_DrawRectMultitextured, rc_draw_rect_multitextured);                                           \
    Action(RenderCommand_DrawLine, rc_draw_line) Action(RenderCommand_LoadTexture, rc_load_texture);                   \
    Action(RenderCommand_LoadTextureRaw, rc_load_texture_raw);                                                         \
    Action(RenderCommand_LoadFont, rc_load_font);                                                                      \
    Action(RenderCommand_SetCameraMatrix, rc_set_camera_matrix);

typedef struct rc_header
{
    rc_kind Kind;
} rc_header;

typedef struct rc_draw_rect
{
    vec2 Mid;
    vec2 Size;
    real32 Rotation;
    vec4 Color;
    texture_slot* TextureSlot;
    vec2 MinUV;
    vec2 MaxUV;
} rc_draw_rect;

typedef struct rc_draw_rect_outline
{
    vec2 Mid;
    vec2 Size;
    real32 Rotation;
    vec4 Color;
} rc_draw_rect_outline;

typedef struct rc_draw_rect_multitextured
{
    vec2 Mid;
    vec2 Size;
    real32 Rotation;
    vec4 Color;
    texture_slot* TextureSlot[4];
    vec2 MinUV[4];
    vec2 MaxUV[4];
} rc_draw_rect_multitextured;

typedef struct rc_draw_line
{
    vec2 Start;
    vec2 End;
    vec4 Color;
} rc_draw_line;

typedef struct rc_load_texture
{
    texture_slot* TextureSlot;
    bool32 Interpolate;
    char* Filename;
} rc_load_texture;

typedef struct rc_load_texture_raw
{
    texture_slot* TextureSlot;
    bool32 Interpolate;
    uint8* Data;
    uint32 Width;
    uint32 Height;
} rc_load_texture_raw;

typedef struct rc_load_font
{
    font_slot* FontSlot;
    char* Filename;
    float Height;
    uint32 OversampleX;
    uint32 OversampleY;
} rc_load_font;

typedef struct rc_draw_string
{
    font_slot* FontSlot;
    vec2 Pos;
    vec4 Color;
    float Scale;
    uint16 CharCount;
} rc_draw_string;

typedef struct rc_set_camera_matrix
{
    mat4 Matrix;
} rc_set_camera_matrix;

internal rc_kind
GetNextKind(render_buffer* RenderBuffer);
internal void*
GetNextCommand(render_buffer* RenderBuffer, rc_kind Kind);

#define RewindRenderBuffer(RenderBuffer) (RenderBuffer)->Current = (RenderBuffer)->Base
#define EndRenderBuffer(RenderBuffer)                                                                                  \
    (RenderBuffer)->End = (RenderBuffer)->Current;                                                                     \
    RewindRenderBuffer(RenderBuffer)

internal rc_draw_rect*
PushDrawRect(render_buffer* RenderBuffer, vec2 Mid, vec2 Size, texture_slot* TextureSlot);

#define LUDUS_RENDERER_H
#endif
