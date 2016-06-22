/*
    Creation Date: 30-04-2016
    Creator: Zakary Strange
*/

internal void
Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state* OldState,
                                DWORD ButtonBit,
                                game_button_state* NewState)
{
    NewState->EndedDown           = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;

    if(Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if(Value > DeadZoneThreshold)
    {
        Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }

    return (Result);
}

internal void
Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
    MSG Message;

    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            }
            break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;

                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown  = ((Message.lParam & (1 << 31)) == 0);

                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveForward, IsDown);
                    }

                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }

                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveBackward, IsDown);
                    }

                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }

                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->DPadUp, IsDown);
                    }

                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->DPadLeft, IsDown);
                    }

                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->DPadDown, IsDown);
                    }

                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->DPadRight, IsDown);
                    }

                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }

// NOTE(zak): Bind A, B, X, Y on controller to buttons on keyboard

#if LUDUS_INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if(VKCode == VK_F1)
                    {
                        if(IsDown)
                        {
                            GlobalDebugMode = !GlobalDebugMode;
                        }
                    }
#endif

                    else if(IsDown)
                    {
                        b32 AltKeyWasDown = (Message.lParam & (1 << 29));
                        if((VKCode == VK_F4) && AltKeyWasDown)
                        {
                            GlobalRunning = false;
                        }

                        if((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            Win32ToggleFullscreen(Message.hwnd);
                        }
                    }
                }
            }
            break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
            break;
        }
    }
}
