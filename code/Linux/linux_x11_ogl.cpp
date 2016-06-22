/*
    Creation Date: 01-05-2016
    Creator: Alex Baines
*/

#define GL_LOAD_PROC glXGetProcAddressARB
#define GL_LOAD_PROC_CAST const GLubyte*
#include "ludus_opengl.cpp"

internal void
LinuxX11OpenGLInit(Display* Disp, XVisualInfo** Vis, GLXFBConfig** FBConf)
{
    int GlxErrorBase = 0;
    int GlxEventBase = 0;
    Bool GlxExists   = glXQueryExtension(Disp, &GlxErrorBase, &GlxEventBase); // no idea what those last two do

    if(!GlxExists)
    {
        LinuxX11ErrorExit("GLX could not be detected; exiting");
    }

    int GlxMajor          = 0;
    int GlxMinor          = 0;
    b32 GlxVersionSuccess = glXQueryVersion(Disp, &GlxMajor, &GlxMinor);

    if(GlxVersionSuccess)
    {
        LinuxLogF("Available GLX version: %i.%i", GlxMajor, GlxMinor);
        if(GlxMajor != 0 && GlxMinor != 0)
        {
            if(GlxMajor >= 1 && GlxMinor >= 4)
            {
                LinuxLogF("GLX version OK");
            }
            else
            {
                LinuxX11ErrorExit("GLX Version: 1.4 reqired");
            }
        }
        else
        {
            LinuxX11PlatformLog("GLX avilable; Version could not be retrieved");
        }
    }
    else
    {
        LinuxX11ErrorExit("GLX version could not be queried");
    }

    int DefaultScreen         = DefaultScreen(Disp);
    const char* GlxExtensions = glXQueryExtensionsString(Disp, DefaultScreen);

    // check for needed glx extensions here, then set the flag
    int GlxFlags = 0;
    if(strstr(GlxExtensions, "GLX_EXT_swap_control"))
    {
        GlxFlags = FLAG_GLX_EXT_swap_control | GlxFlags;
    }

    int SamplesEnum = None;
    if(strstr(GlxExtensions, "GLX_ARB_multisample "))
    {
        SamplesEnum = GLX_SAMPLE_BUFFERS_ARB;
    }

    int GLXFBAttribs[] = {GLX_DRAWABLE_TYPE,
                          GLX_WINDOW_BIT,
                          GLX_RENDER_TYPE,
                          GLX_RGBA_BIT,
                          GLX_RED_SIZE,
                          8,
                          GLX_GREEN_SIZE,
                          8,
                          GLX_BLUE_SIZE,
                          8,
                          GLX_ALPHA_SIZE,
                          8,
                          GLX_DEPTH_SIZE,
                          8,
                          GLX_DOUBLEBUFFER,
                          True,
                          SamplesEnum,
                          1,
                          GLX_SAMPLES_ARB,
                          4,
                          None};

    int GlxFbconfigElemc;
    GLXFBConfig* GlxFbconfig = glXChooseFBConfig(Disp, DefaultScreen, GLXFBAttribs, &GlxFbconfigElemc);

    if(!GlxFbconfig)
    {
        LinuxX11ErrorExit("No GLX FB config could be retrieved; exiting");
    }
    else
    {
        LinuxLogF("Got %i FB configs", GlxFbconfigElemc);
    }

    int FBCID;
    glXGetFBConfigAttrib(Disp, *GlxFbconfig, GLX_FBCONFIG_ID, &FBCID);
    LinuxLogF("Using FBConfig with ID: %d (0x%x)", FBCID, FBCID);

    XVisualInfo* VisInfo = glXGetVisualFromFBConfig(Disp, *GlxFbconfig);

    if(!VisInfo)
    {
        LinuxX11ErrorExit("XVisualInfo could not be retrieved; exiting");
    }

    *Vis    = VisInfo;
    *FBConf = GlxFbconfig;
}

internal GLXContext
LinuxX11OpenGLCreateContext(Display* Disp, Window Win, GLXFBConfig* FBConf)
{
    int GlxGlAttr[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB,
        3,
        GLX_CONTEXT_MINOR_VERSION_ARB,
        1,
        GLX_CONTEXT_PROFILE_MASK_ARB,
        GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
#if LUDUS_INTERNAL
        GLX_CONTEXT_FLAGS_ARB,
        GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
        None
    };

    glXCreateContextAttribsARB =
        (glx_create_context_attribs_arb*)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    if(!glXCreateContextAttribsARB)
    {
        LinuxX11ErrorExit("glXCreateContextAttribsARB is not available; exiting");
    }

    GLXContext GLContext = glXCreateContextAttribsARB(Disp, *FBConf, NULL, true, GlxGlAttr);

    if(!GLContext)
    {
        LinuxX11ErrorExit("Opengl 3.1 direct rendering context not available; exiting");
    }

    glXMakeCurrent(Disp, Win, GLContext);

    return GLContext;
}
