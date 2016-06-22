#define HANDMADE_MATH_CPP_MODE
#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"
#undef HANDMADE_MATH_IMPLEMENTATION

#include "game.h"
#include "ludus_animate.cpp"
#include "ludus_entity.cpp"
#include "ludus_renderer.cpp"
#include "ludus_shared.cpp"

extern "C" GAME_GENERATE_AUDIO_SAMPLES(GameGenerateAudioSamples)
{
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    PushSoundSamples(&GameState->AudioState, SampleRequest);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Platform = Memory->PlatformAPI;

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        GameState->IsInitialized = true;
    }

    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    if(!TranState->IsInitialized)
    {
        InitializeArena(&TranState->Arena,
                        (memory_index)(Memory->TransientStorageSize - sizeof(transient_state)),
                        (uint8*)Memory->TransientStorage + sizeof(transient_state));

        TranState->IsInitialized = true;
    }


    // Update Input
    for(u32 ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input* Controller = GetController(Input, ControllerIndex);

        if(Controller->IsAnalog)
        {
            // NOTE(user): Use analog movement tuning
        }
        else
        {
            // NOTE(user): Use digital movement tuning
        }

        // NOTE(user): Check controller button state here
    }

    // NOTE(user): Update and Render down here
    RenderBuffer->ClearColor = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
}