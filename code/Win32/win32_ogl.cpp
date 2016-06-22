/*
    Creation Date: 30-04-2016
    Creator: Zakary Strange
*/

#define USE_OGL_RENDERER 1

typedef HGLRC WINAPI
wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext, const int* attribList);

typedef BOOL WINAPI
wgl_swap_interval_ext(int interval);

typedef BOOL WINAPI
wgl_choose_pixel_format_arb(
    HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);

global_variable wgl_swap_interval_ext* wglSwapInterval;
global_variable wgl_choose_pixel_format_arb* wglChoosePixelFormatARB;
global_variable wgl_create_context_attribs_arb* wglCreateContextAttribsARB;

internal void
Win32SetPixelFormat(HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick           = 0;

    if(wglChoosePixelFormatARB)
    {
        int AttribList[] = {
            WGL_DRAW_TO_WINDOW_ARB,
            GL_TRUE,
            WGL_ACCELERATION_ARB,
            WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB,
            GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,
            GL_TRUE,
            WGL_PIXEL_TYPE_ARB,
            WGL_TYPE_RGBA_ARB,
            WGL_SAMPLE_BUFFERS_ARB,
            GL_TRUE,
            WGL_SAMPLES_ARB,
            4,
            0,
        };

        wglChoosePixelFormatARB(WindowDC, AttribList, 0, 1, &SuggestedPixelFormatIndex, &ExtendedPick);
    }

    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {0};
        DesiredPixelFormat.nSize                 = sizeof(PIXELFORMATDESCRIPTOR);
        DesiredPixelFormat.nVersion              = 1;
        DesiredPixelFormat.iPixelType            = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags               = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits            = 32;
        DesiredPixelFormat.cAlphaBits            = 8;
        DesiredPixelFormat.iLayerType            = PFD_MAIN_PLANE;

        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions()
{
    WNDCLASSA WindowClass     = {0};
    WindowClass.lpfnWndProc   = DefWindowProcA;
    WindowClass.hInstance     = GetModuleHandle(0);
    WindowClass.lpszClassName = "LudusWGLLoader";

    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "Ludus WGL Loader",
                                      0,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      WindowClass.hInstance,
                                      0);
        if(Window)
        {
            HDC WindowDC = GetDC(Window);
            Win32SetPixelFormat(WindowDC);
            HGLRC OpenGLRC = wglCreateContext(WindowDC);

            if(wglMakeCurrent(WindowDC, OpenGLRC))
            {
                wglSwapInterval = (wgl_swap_interval_ext*)wglGetProcAddress("wglSwapIntervalEXT");
                if(!wglSwapInterval)
                {
                    Win32Log("Couldnt get wglSwapInternalEXT extension");
                }

                wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb*)wglGetProcAddress("wglChoosePixelFormatARB");
                if(!wglChoosePixelFormatARB)
                {
                    Win32Log("Couldnt get wglChoosePixelFormatARB extension");
                }

                wglCreateContextAttribsARB =
                    (wgl_create_context_attribs_arb*)wglGetProcAddress("wglCreateContextAttribsARB");
                if(!wglCreateContextAttribsARB)
                {
                    Win32Log("Couldnt get wglCreateContextAttribsARB extension");
                }

                wglMakeCurrent(0, 0);
            }

            wglDeleteContext(OpenGLRC);
            ReleaseDC(Window, WindowDC);
            DestroyWindow(Window);
        }
        else
        {
            Win32Log("Couldnt create WGL Loader Window");
            LOG_FORMATTED_ERROR(4096);
        }
    }
    else
    {
        Win32Log("Couldnt register class for WGL Loader Window");
        LOG_FORMATTED_ERROR(4096);
    }
}

internal void
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();

    HGLRC OpenGLRC       = 0;
    bool32 ModernContext = true;

    if(wglCreateContextAttribsARB)
    {
        int Win32OpenGLAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB,
            3,
            WGL_CONTEXT_MINOR_VERSION_ARB,
            1,
            WGL_CONTEXT_FLAGS_ARB,
            WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if LUDUS_INTERNAL
                |
                WGL_CONTEXT_DEBUG_BIT_ARB
#endif
            ,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };

        Win32SetPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }

    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC      = wglCreateContext(WindowDC);
    }

    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        InitOpenGL(ModernContext);

        if(wglSwapInterval)
        {
            wglSwapInterval(1);
        }
    }
    else
    {
        Win32Log("Couldnt set OpenGL context");
        LOG_FORMATTED_ERROR(4096);
    }
}
