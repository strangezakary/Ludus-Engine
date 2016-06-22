/*
    Creation Date: 26-03-2016
    Creator: Zakary Strange
*/

#include "ludus_opengl.h"
#include "ludus_renderer.h"

// TODO(kiljacken): Remove the malloc, realloc and free deps
#define STBI_NO_STDIO
#define STBI_ASSERT Assert
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO(kiljacken): Override qsort dep
#define STBRP_ASSERT Assert
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STBTT_malloc(x, u) ((void)(u), Platform.AllocateMemory(x))
#define STBTT_free(x, u) ((void)(u), Platform.DeallocateMemory(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

/*
  NOTE(kiljacken): Before including this header in your platform layer,
  you must define the following:
  - GL_LOAD_PROC: You're platform specific gl proc loader
    (wglGetProcAddress, glXGetProcAddress, etc.)
  - GL_LOAD_PROC_CAST: The type used for the pname string in the
    proc loader.
    (LPCSTR on windows, const GLubyte * on linux, etc.)
*/

#if !defined(GL_LOAD_PROC) || !defined(GL_LOAD_PROC_CAST)
#error GL_LOAD_PROC or GL_LOAD_PROC_CAST  undefined, read top comment in ludus_opengl.cpp for more info.
#endif

internal opengl_info
OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result = {};

    Result.ModernContext = ModernContext;
    Result.Vendor        = (char*)glGetString(GL_VENDOR);
    Result.Renderer      = (char*)glGetString(GL_RENDERER);
    Result.Version       = (char*)glGetString(GL_VERSION);

    // NOTE(inso): I don't think these version params are available on < 3.0 contexts, would need to
    // parse the version string in Info instead.

    Result.MajorVersion = Result.MinorVersion = 0;

    glGetIntegerv(GL_MAJOR_VERSION, &Result.MajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &Result.MinorVersion);

    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        // NOTE(inso): we could get shaders on legacy 2.1 if the extensions are present.
        Result.ShadingLanguageVersion = "none";
    }

    return (Result);
}

internal void
OpenGLErrorCallback(
    GLenum Source, GLenum Type, GLuint Id, GLenum Severity, GLsizei Length, char* Message, void* UserParam)
{
    if(Severity != DEBUG_SEVERITY_HIGH)
    {
        return;
    }

    if(Platform.OutputDebugString)
    {
        Platform.OutputDebugString(Message);
    }
#if 1
    if(Platform.Log)
    {
        Platform.Log(Message);
    }
#endif
}

#define OpenGLLoadAndCheck(Type, Name)                                                                                 \
    {                                                                                                                  \
        gl.Name = (Type*)GL_LOAD_PROC((GL_LOAD_PROC_CAST) #Name);                                                      \
        if(!gl.Name)                                                                                                   \
        {                                                                                                              \
            Platform.Log("Unable to load gl function: " #Name);                                                        \
        }                                                                                                              \
    }

internal void
InitOpenGL(b32 ModernContext)
{
    opengl_info Info = OpenGLGetInfo(ModernContext);

    Platform.Log(Info.Vendor);
    Platform.Log(Info.Renderer);
    Platform.Log(Info.Version);
    Platform.Log(Info.ShadingLanguageVersion);

    // NOTE(inso): need to load this func early since CheckExtensions needs it.
    OpenGLLoadAndCheck(gl_get_stringi, glGetStringi);

    OpenGLCheckExtensions(&Info);
    LoadOpenGLFunctions(&Info);

#if LUDUS_INTERNAL
    if(Info.KHR_debug || Info.MajorVersion > 4 || (Info.MajorVersion == 4 && Info.MinorVersion >= 3))
    {
        gl.glDebugMessageCallback((gl_debug_proc*)&OpenGLErrorCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        Platform.Log("Using GL_KHR_debug for gl debug output");
    }
    else if(Info.ARB_debug_output)
    {
        gl.glDebugMessageCallbackARB((gl_debug_proc*)&OpenGLErrorCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        Platform.Log("Using GL_ARB_debug_output for gl debug output");
    }
    else
    {
        Platform.Log("No gl debug output extension available");
    }
#endif
}

internal void
LoadOpenGLFunctions(opengl_info* Info)
{
    // NOTE(inso): the name aliasing of the gl funcs might be a problem on linux
    // read Misconception #4 here: https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

    OpenGLLoadAndCheck(gl_gen_buffers, glGenBuffers);
    OpenGLLoadAndCheck(gl_bind_buffer, glBindBuffer);
    OpenGLLoadAndCheck(gl_buffer_data, glBufferData);
    OpenGLLoadAndCheck(gl_create_shader, glCreateShader);
    OpenGLLoadAndCheck(gl_shader_source, glShaderSource);
    OpenGLLoadAndCheck(gl_compile_shader, glCompileShader);
    OpenGLLoadAndCheck(gl_get_shaderiv, glGetShaderiv);
    OpenGLLoadAndCheck(gl_get_shader_info_log, glGetShaderInfoLog);
    OpenGLLoadAndCheck(gl_create_program, glCreateProgram);
    OpenGLLoadAndCheck(gl_attach_shader, glAttachShader);
    OpenGLLoadAndCheck(gl_bind_frag_data_location, glBindFragDataLocation);
    OpenGLLoadAndCheck(gl_link_program, glLinkProgram);
    OpenGLLoadAndCheck(gl_use_program, glUseProgram);
    OpenGLLoadAndCheck(gl_get_program_iv, glGetProgramiv);
    OpenGLLoadAndCheck(gl_get_program_info_log, glGetProgramInfoLog);
    OpenGLLoadAndCheck(gl_get_attrib_location, glGetAttribLocation);
    OpenGLLoadAndCheck(gl_vertex_attrib_pointer, glVertexAttribPointer);
    OpenGLLoadAndCheck(gl_enable_vertex_attrib_array, glEnableVertexAttribArray);
    OpenGLLoadAndCheck(gl_disable_vertex_attrib_array, glDisableVertexAttribArray);
    OpenGLLoadAndCheck(gl_gen_vertex_arrays, glGenVertexArrays);
    OpenGLLoadAndCheck(gl_bind_vertex_array, glBindVertexArray);
    OpenGLLoadAndCheck(gl_get_uniform_location, glGetUniformLocation);
    OpenGLLoadAndCheck(gl_delete_vertex_arrays, glDeleteVertexArrays);
    OpenGLLoadAndCheck(gl_delete_buffers, glDeleteBuffers);
    OpenGLLoadAndCheck(gl_delete_shader, glDeleteShader);
    OpenGLLoadAndCheck(gl_delete_program, glDeleteProgram);
    OpenGLLoadAndCheck(gl_detach_shader, glDetachShader);
    OpenGLLoadAndCheck(gl_uniform_matrix4fv, glUniformMatrix4fv);
    OpenGLLoadAndCheck(gl_buffer_sub_data, glBufferSubData);
    OpenGLLoadAndCheck(gl_active_texture, glActiveTexture);
    OpenGLLoadAndCheck(gl_uniform1i, glUniform1i);
    OpenGLLoadAndCheck(gl_uniform4f, glUniform4f);

// NOTE(kiljacken): Everything following this comment is an extension, and might be null

// NOTE(inso): On linux you should only check these if you know they were in the extension string(s)
// if it's not available glXGetProcAddress can still return non-null but it'll break.
// see https://dri.freedesktop.org/wiki/glXGetProcAddressNeverReturnsNULL/

#if LUDUS_INTERNAL
    if(Info->KHR_debug || Info->MajorVersion > 4 || (Info->MajorVersion == 4 && Info->MinorVersion >= 3))
    {
        OpenGLLoadAndCheck(gl_debug_message_callback, glDebugMessageCallback);
    }
    else if(Info->ARB_debug_output)
    {
        OpenGLLoadAndCheck(gl_debug_message_callback_arb, glDebugMessageCallbackARB);
    }
#endif
}

internal void
OpenGLCheckExtensions(opengl_info* Info)
{
    GLint NumExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &NumExtensions);
    for(int i = 0; i < NumExtensions; ++i)
    {
        char* Extension = (char*)gl.glGetStringi(GL_EXTENSIONS, i);
        if(!Extension)
            continue;
#if 0
        Platform.Log(Extension);
#endif

        if(StringsAreEqual(Extension, "GL_KHR_debug"))
        {
            Platform.Log("Found GL_KHR_debug");
            Info->KHR_debug = true;
        }
        else if(StringsAreEqual(Extension, "GL_ARB_debug_output"))
        {
            Platform.Log("Found GL_ARB_debug_output");
            Info->ARB_debug_output = true;
        }
    }
}

// TODO(zak): Move this out to a platform independent layer.
// To support generic hot-reloading later on.
internal inline b32
CheckForReload(char* FilePath, s64* OldFileWriteTime)
{
    s64 NewFileWriteTime = Platform.GetFileTime(FilePath);
    if(*OldFileWriteTime != NewFileWriteTime)
    {
        OldFileWriteTime = &NewFileWriteTime;
        return true;
    }

    return false;
}

/*
   So im just gonna write out here how your gonna wanna do the shader hot-reloading.
   As i did not want to change around the whole project structure since i know you were doing things.
   So basically this is how i would do it / have done it in another project.
   Please note that i know that this can be done better i just wanted to get you a quick
   and simple solution:


   shader VertexShader = {};
   char *VertexShaderPath = "Shaders/test.vert";
   if(CheckForReload(VertexShaderPath, &VertexShader.OldFileWriteTime))
   {
       VertexShader = OpenGLLoadShader(VertexShaderPath, GL_VERTEX_SHADER);
       VertexShader.IsInitialized = true;
   }

   shader FragmentShader = {};
   char *FragmentShaderPath = "Shaders/test.vert";
   if(CheckForReload(FragmentShaderPath, &FragmentShader.OldFileWriteTime))
   {
       FragmentShader = OpenGLLoadShader(FragmentShaderPath, GL_VERTEX_SHADER);
       FragmentShader.IsInitialized = true;
   }

   GLuint ShaderLinkID;
   if((FragmentShader.IsInitialized) || (VertexShader.IsInitialized)
   {
      ShaderLinkID = OpenGLLinkShader(VertexShader, FragmentShader);
      VertexShader.IsInitialized = false;
      FragmentShader.IsInitialized = false;
   }

   I know this doesnt really work in pratice with controlling state and all,
   but what i would do is just throw all the shader handles into the gamestate and it
   should solve all the problems. Although this is not the perfect solution it should
   get you somewhere.

*/

internal shader
OpenGLLoadShader(char* FilePath, uint32 ShaderType)
{
    shader Result = {};

    read_file_result File   = Platform.ReadEntireFile(FilePath);
    Result.OldFileWriteTime = Platform.GetFileTime(FilePath);

    if(File.Contents)
    {
        Result.ID = gl.glCreateShader(ShaderType);

        gl.glShaderSource(Result.ID, 1, (GLchar**)&File.Contents, 0);
        gl.glCompileShader(Result.ID);

        GLint ShaderCompileStatus = 0;
        gl.glGetShaderiv(Result.ID, GL_COMPILE_STATUS, &ShaderCompileStatus);

        if(ShaderCompileStatus == GL_FALSE)
        {
            GLint ShaderInfoLogLength = 0;
            gl.glGetShaderiv(Result.ID, GL_INFO_LOG_LENGTH, &ShaderInfoLogLength);

            if(ShaderInfoLogLength > 1)
            {
                GLchar* ShaderInfoLog = (GLchar*)Platform.AllocateMemory(ShaderInfoLogLength);
                gl.glGetShaderInfoLog(Result.ID, ShaderInfoLogLength, &ShaderInfoLogLength, ShaderInfoLog);
                Platform.Log(FilePath);
                Platform.Log(ShaderInfoLog);
                Platform.DeallocateMemory(ShaderInfoLog);
            }
            else
            {
                Platform.Log("LoadShader: Your info log was too short");
            }
        }

        Platform.FreeFileMemory(File.Contents);
    }
    else
    {
        Platform.Log(FilePath);
        Platform.Log("Shader returned no contents");
    }

    return (Result);
}

internal void
OpenGLLinkShader(GLuint ShaderProgram, GLuint ShaderOne, GLuint ShaderTwo)
{
    gl.glAttachShader(ShaderProgram, ShaderOne);
    gl.glAttachShader(ShaderProgram, ShaderTwo);
    gl.glLinkProgram(ShaderProgram);

    GLint ProgramLinkStatus = 0;
    gl.glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &ProgramLinkStatus);
    if(ProgramLinkStatus == GL_FALSE)
    {
        GLint ProgramInfoLogLength = 0;
        gl.glGetProgramiv(ShaderProgram, GL_INFO_LOG_LENGTH, &ProgramInfoLogLength);
        if(ProgramInfoLogLength > 1)
        {
            GLchar* ProgramInfoLog = (GLchar*)Platform.AllocateMemory(ProgramInfoLogLength);
            gl.glGetProgramInfoLog(ShaderProgram, ProgramInfoLogLength, &ProgramInfoLogLength, ProgramInfoLog);
            Platform.Log(ProgramInfoLog);
            Platform.DeallocateMemory(ProgramInfoLog);
        }
        else
        {
            Platform.Log("LinkShader: Your info log was too short");
        }
    }
    else
    {
        gl.glDetachShader(ShaderProgram, ShaderOne);
        gl.glDetachShader(ShaderProgram, ShaderTwo);
    }
}

internal void
OpenGLUpdateSize(u32 Width, u32 Height)
{
    glViewport(0, 0, Width, Height);
    GLState.ProjectionMatrix = Orthographic(0.0f, (float)Width, (float)Height, 0.0f, 0.0f, 100.0f);
}

internal bool32
OpenGLCheckLoadShader(shader* VertexShader,
                      shader* FragmentShader,
                      GLuint* ShaderProgram,
                      char* VertexShaderPath,
                      char* FragmentShaderPath)
{
    bool32 Result = false;
    if(CheckForReload(VertexShaderPath, &VertexShader->OldFileWriteTime))
    {
        if(VertexShader->ID)
        {
            gl.glDeleteShader(VertexShader->ID);
        }

        *VertexShader               = OpenGLLoadShader(VertexShaderPath, GL_VERTEX_SHADER);
        VertexShader->IsInitialized = false;
    }

    if(CheckForReload(FragmentShaderPath, &FragmentShader->OldFileWriteTime))
    {
        if(FragmentShader->ID)
        {
            gl.glDeleteShader(FragmentShader->ID);
        }

        *FragmentShader               = OpenGLLoadShader(FragmentShaderPath, GL_FRAGMENT_SHADER);
        FragmentShader->IsInitialized = false;
    }

    if((!FragmentShader->IsInitialized) || (!VertexShader->IsInitialized))
    {
        Result = true;
        if(*ShaderProgram)
        {
            gl.glDeleteProgram(*ShaderProgram);
        };

        *ShaderProgram = gl.glCreateProgram();
        gl.glBindFragDataLocation(*ShaderProgram, 0, "outColor");

        OpenGLLinkShader(*ShaderProgram, VertexShader->ID, FragmentShader->ID);
        VertexShader->IsInitialized   = true;
        FragmentShader->IsInitialized = true;
    }
    return (Result);
}

internal bool32
OpenGLSetupVAO(GLuint* VaoId, GLuint* VboId, memory_index Size)
{
    bool32 Result = false;
    if(!*VaoId)
    {
        gl.glGenVertexArrays(1, VaoId);
        Result = true;
    }
    gl.glBindVertexArray(*VaoId);

    if(!*VboId)
    {
        gl.glGenBuffers(1, VboId);
    }

    gl.glBindBuffer(GL_ARRAY_BUFFER, *VboId);
    gl.glBufferData(GL_ARRAY_BUFFER, Size, 0, GL_STREAM_DRAW);
    return (Result);
}

internal void
OpenGLRenderBuffer(render_buffer* RenderBuffer)
{
    uint32 TotalCount[RenderCommandKindCount] = {0};
    while(RenderBuffer->Current < RenderBuffer->End)
    {
        rc_kind Kind = GetNextKind(RenderBuffer);
        GetNextCommand(RenderBuffer, Kind);
        ++TotalCount[Kind];
    }
    RewindRenderBuffer(RenderBuffer);

    glClearColor(
        RenderBuffer->ClearColor.R, RenderBuffer->ClearColor.G, RenderBuffer->ClearColor.B, RenderBuffer->ClearColor.A);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if(!GLState.AlphaZeroTexture)
    {
        glGenTextures(1, &GLState.AlphaZeroTexture);
        glBindTexture(GL_TEXTURE_2D, GLState.AlphaZeroTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        uint32 Pixels[] = {0x00000000};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if(!GLState.WhiteTexture)
    {
        glGenTextures(1, &GLState.WhiteTexture);
        glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        uint32 Pixels[] = {0xffffffff};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if(OpenGLCheckLoadShader(&GLState.SimpleVertexShader,
                             &GLState.SimpleFragmentShader,
                             &GLState.SimpleShaderProgram,
                             "Shaders/simple.vert",
                             "Shaders/simple.frag"))
    {
        GLState.SimpleShaderMVP   = gl.glGetUniformLocation(GLState.SimpleShaderProgram, "MVP");
        GLState.SimpleShaderColor = gl.glGetUniformLocation(GLState.SimpleShaderProgram, "color");
    }

    if(OpenGLCheckLoadShader(&GLState.MultiVertexShader,
                             &GLState.MultiFragmentShader,
                             &GLState.MultiShaderProgram,
                             "Shaders/multi.vert",
                             "Shaders/multi.frag"))
    {
        GLState.MultiShaderMVP      = gl.glGetUniformLocation(GLState.MultiShaderProgram, "MVP");
        GLState.MultiShaderColor    = gl.glGetUniformLocation(GLState.MultiShaderProgram, "color");
        GLState.MultiShaderTexture0 = gl.glGetUniformLocation(GLState.MultiShaderProgram, "textures[0]");
        GLState.MultiShaderTexture1 = gl.glGetUniformLocation(GLState.MultiShaderProgram, "textures[1]");
        GLState.MultiShaderTexture2 = gl.glGetUniformLocation(GLState.MultiShaderProgram, "textures[2]");
        GLState.MultiShaderTexture3 = gl.glGetUniformLocation(GLState.MultiShaderProgram, "textures[3]");
    }

    if(OpenGLCheckLoadShader(&GLState.TextVertexShader,
                             &GLState.TextFragmentShader,
                             &GLState.TextShaderProgram,
                             "Shaders/text.vert",
                             "Shaders/text.frag"))
    {
        GLState.TextShaderMVP   = gl.glGetUniformLocation(GLState.TextShaderProgram, "MVP");
        GLState.TextShaderColor = gl.glGetUniformLocation(GLState.TextShaderProgram, "color");
    }

    if(OpenGLSetupVAO(&GLState.SimpleVertexArray,
                      &GLState.SimpleVertexBuffer,
                      TotalCount[RenderCommand_DrawRect] * 6 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    if(OpenGLSetupVAO(&GLState.OutlineVertexArray,
                      &GLState.OutlineVertexBuffer,
                      TotalCount[RenderCommand_DrawRectOutline] * 8 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    if(OpenGLSetupVAO(&GLState.MultiVertexArray,
                      &GLState.MultiVertexBuffer,
                      TotalCount[RenderCommand_DrawRectMultitextured] * 6 * 10 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.MultiShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), 0);
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib1 = gl.glGetAttribLocation(GLState.MultiShaderProgram, "vertTexCoord1");
        gl.glVertexAttribPointer(
            TexCoordAttrib1, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib1);

        GLint TexCoordAttrib2 = gl.glGetAttribLocation(GLState.MultiShaderProgram, "vertTexCoord2");
        gl.glVertexAttribPointer(
            TexCoordAttrib2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(4 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib2);

        GLint TexCoordAttrib3 = gl.glGetAttribLocation(GLState.MultiShaderProgram, "vertTexCoord3");
        gl.glVertexAttribPointer(
            TexCoordAttrib3, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib3);

        GLint TexCoordAttrib4 = gl.glGetAttribLocation(GLState.MultiShaderProgram, "vertTexCoord4");
        gl.glVertexAttribPointer(
            TexCoordAttrib4, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(8 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib4);
    }

    if(OpenGLSetupVAO(&GLState.LineVertexArray,
                      &GLState.LineVertexBuffer,
                      TotalCount[RenderCommand_DrawLine] * 2 * 4 * sizeof(float)))
    {
        GLint PosAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "position");
        gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        gl.glEnableVertexAttribArray(PosAttrib);

        GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.SimpleShaderProgram, "vertTexCoord");
        gl.glVertexAttribPointer(TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl.glEnableVertexAttribArray(TexCoordAttrib);
    }

    GLState.ViewMatrix        = Mat4d(1.0f);
    mat4 ViewProjectionMatrix = GLState.ProjectionMatrix * GLState.ViewMatrix;

    uint32 SimpleRectCount  = 0;
    uint32 OutlineRectCount = 0;
    uint32 MultiRectCount   = 0;
    uint32 LineCount        = 0;

    while(RenderBuffer->Current < RenderBuffer->End)
    {
        rc_kind Kind = GetNextKind(RenderBuffer);
        void* Data   = GetNextCommand(RenderBuffer, Kind);

        switch(Kind)
        {
            case RenderCommand_DrawRect:
            {
                rc_draw_rect* Command = (rc_draw_rect*)Data;

                mat4 TranlationM = Translate(Vec3(Command->Mid.X, Command->Mid.Y, 0.0f));
                mat4 ScaleM      = Scale(Vec3(Command->Size.X, Command->Size.Y, 0.0f));
                mat4 RotateM     = Rotate(Command->Rotation, Vec3(0.0f, 0.0f, 1.0f));
                mat4 ModelMatrix = TranlationM * RotateM * ScaleM;
                mat4 MVP         = ViewProjectionMatrix * ModelMatrix;

                if(Command->TextureSlot)
                {
                    vec2 HalfPixel =
                        Vec2(0.5f / (float)Command->TextureSlot->Width, 0.5f / (float)Command->TextureSlot->Height);
                    Command->MinUV = Command->MinUV + HalfPixel;
                    Command->MaxUV = Command->MaxUV - HalfPixel;
                }

                float Vertices[] = {
                    -0.5f, -0.5f, Command->MinUV.X, Command->MinUV.Y, -0.5f, 0.5f, Command->MinUV.X, Command->MaxUV.Y,
                    0.5f,  0.5f,  Command->MaxUV.X, Command->MaxUV.Y,

                    -0.5f, -0.5f, Command->MinUV.X, Command->MinUV.Y, 0.5f,  0.5f, Command->MaxUV.X, Command->MaxUV.Y,
                    0.5f,  -0.5f, Command->MaxUV.X, Command->MinUV.Y,
                };

                gl.glBindVertexArray(GLState.SimpleVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.SimpleVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, SimpleRectCount * sizeof(Vertices), sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                if(Command->TextureSlot)
                {
                    GLuint TextureHandle = *(GLuint*)&Command->TextureSlot->ImplData;
                    glBindTexture(GL_TEXTURE_2D, TextureHandle);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
                }

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(
                    GLState.SimpleShaderColor, Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);

                glDrawArrays(GL_TRIANGLES, SimpleRectCount * 6, 6);

                ++SimpleRectCount;
            }
            break;

            case RenderCommand_DrawRectOutline:
            {
                rc_draw_rect_outline* Command = (rc_draw_rect_outline*)Data;

                mat4 TranlationM = Translate(Vec3(Command->Mid.X, Command->Mid.Y, 0.0f));
                mat4 ScaleM      = Scale(Vec3(Command->Size.X, Command->Size.Y, 0.0f));
                mat4 RotateM     = Rotate(Command->Rotation, Vec3(0.0f, 0.0f, 1.0f));
                mat4 ModelMatrix = TranlationM * RotateM * ScaleM;
                mat4 MVP         = ViewProjectionMatrix * ModelMatrix;

                float Vertices[] = {
                    -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,

                    0.5f,  -0.5f, 1.0f, 0.0f, 0.5f,  0.5f,  1.0f, 1.0f,

                    0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f,

                    -0.5f, 0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f,
                };

                gl.glBindVertexArray(GLState.OutlineVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.OutlineVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, OutlineRectCount * sizeof(Vertices), sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(
                    GLState.SimpleShaderColor, Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);

                glDrawArrays(GL_LINES, OutlineRectCount * 8, 8);

                ++OutlineRectCount;
            }
            break;

            case RenderCommand_DrawRectMultitextured:
            {
                rc_draw_rect_multitextured* Command = (rc_draw_rect_multitextured*)Data;

                mat4 TranlationM = Translate(Vec3(Command->Mid.X, Command->Mid.Y, 0.0f));
                mat4 ScaleM      = Scale(Vec3(Command->Size.X, Command->Size.Y, 0.0f));
                mat4 RotateM     = Rotate(Command->Rotation, Vec3(0.0f, 0.0f, 1.0f));
                mat4 ModelMatrix = TranlationM * RotateM * ScaleM;
                mat4 MVP         = ViewProjectionMatrix * ModelMatrix;

                for(u64 i = 0; i < ArrayCount(Command->TextureSlot); ++i)
                {
                    if(Command->TextureSlot[i])
                    {
                        vec2 HalfPixel = Vec2(0.5f / (float)Command->TextureSlot[i]->Width,
                                              0.5f / (float)Command->TextureSlot[i]->Height);
                        Command->MinUV[i] = Command->MinUV[i] + HalfPixel;
                        Command->MaxUV[i] = Command->MaxUV[i] - HalfPixel;
                    }
                }

#define TextureCoords(S, s, U, u)                                                                                      \
    Command->S[0].s, Command->U[0].u, Command->S[1].s, Command->U[1].u, Command->S[2].s, Command->U[2].u,              \
        Command->S[3].s, Command->U[3].u

                float Vertices[] = {
                    -0.5f,
                    -0.5f,
                    TextureCoords(MinUV, X, MinUV, Y),
                    -0.5f,
                    0.5f,
                    TextureCoords(MinUV, X, MaxUV, Y),
                    0.5f,
                    0.5f,
                    TextureCoords(MaxUV, X, MaxUV, Y),

                    -0.5f,
                    -0.5f,
                    TextureCoords(MinUV, X, MinUV, Y),
                    0.5f,
                    0.5f,
                    TextureCoords(MaxUV, X, MaxUV, Y),
                    0.5f,
                    -0.5f,
                    TextureCoords(MaxUV, X, MinUV, Y),
                };
#undef TextureCoords

                gl.glBindVertexArray(GLState.MultiVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.MultiVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, MultiRectCount * sizeof(Vertices), sizeof(Vertices), Vertices);

                for(u64 i = 0; i < ArrayCount(Command->TextureSlot); ++i)
                {
                    gl.glActiveTexture(GL_TEXTURE0 + (GLenum)i);
                    if(Command->TextureSlot[i])
                    {
                        GLuint TextureHandle = *(GLuint*)&Command->TextureSlot[i]->ImplData;
                        glBindTexture(GL_TEXTURE_2D, TextureHandle);
                    }
                    else if(i == 0)
                    {
                        glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);
                    }
                    else
                    {
                        glBindTexture(GL_TEXTURE_2D, GLState.AlphaZeroTexture);
                    }
                }

                gl.glUseProgram(GLState.MultiShaderProgram);

                gl.glUniformMatrix4fv(GLState.MultiShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(
                    GLState.MultiShaderColor, Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);
                gl.glUniform1i(GLState.MultiShaderTexture0, 0);
                gl.glUniform1i(GLState.MultiShaderTexture1, 1);
                gl.glUniform1i(GLState.MultiShaderTexture2, 2);
                gl.glUniform1i(GLState.MultiShaderTexture3, 3);

                glDrawArrays(GL_TRIANGLES, MultiRectCount * 6, 6);

                ++MultiRectCount;
            }
            break;

            case RenderCommand_DrawLine:
            {
                rc_draw_line* Command = (rc_draw_line*)Data;

                mat4 MVP = ViewProjectionMatrix;

                float Vertices[] = {
                    Command->Start.X, Command->Start.Y, 0.0f, 0.0f, Command->End.X, Command->End.Y, 1.0f, 1.0f,
                };

                gl.glBindVertexArray(GLState.LineVertexArray);
                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.LineVertexBuffer);
                gl.glBufferSubData(GL_ARRAY_BUFFER, LineCount * sizeof(Vertices), sizeof(Vertices), Vertices);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, GLState.WhiteTexture);

                gl.glUseProgram(GLState.SimpleShaderProgram);

                gl.glUniformMatrix4fv(GLState.SimpleShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(
                    GLState.SimpleShaderColor, Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);

                glDrawArrays(GL_LINES, LineCount * 2, 2);

                ++LineCount;
            }
            break;

            case RenderCommand_LoadTexture:
            {
                rc_load_texture* Command = (rc_load_texture*)Data;

                GLuint* TextureHandle_ = (GLuint*)Command->TextureSlot->ImplData;
                if(!*TextureHandle_)
                {
                    glGenTextures(1, TextureHandle_);
                }

                GLuint TextureHandle = *TextureHandle_;

                glBindTexture(GL_TEXTURE_2D, TextureHandle);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);

                read_file_result ImageFile = Platform.ReadEntireFile(Command->Filename);

                int Width, Height, Comps;
                uint8* IData = stbi_load_from_memory(
                    (uint8*)ImageFile.Contents, ImageFile.ContentsSize, &Width, &Height, &Comps, 4);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, IData);
                stbi_image_free(IData);
                Platform.FreeFileMemory(ImageFile.Contents);

                Command->TextureSlot->Width  = Width;
                Command->TextureSlot->Height = Height;
            }
            break;

            case RenderCommand_LoadTextureRaw:
            {
                rc_load_texture_raw* Command = (rc_load_texture_raw*)Data;

                GLuint* TextureHandle_ = (GLuint*)Command->TextureSlot->ImplData;
                if(!*TextureHandle_)
                {
                    glGenTextures(1, TextureHandle_);
                }

                GLuint TextureHandle = *TextureHandle_;
                glBindTexture(GL_TEXTURE_2D, TextureHandle);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Command->Interpolate ? GL_LINEAR : GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D,
                             0,
                             GL_RGBA,
                             Command->Width,
                             Command->Height,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             Command->Data);
                glBindTexture(GL_TEXTURE_2D, 0);

                Command->TextureSlot->Width  = Command->Width;
                Command->TextureSlot->Height = Command->Height;
            }
            break;

            case RenderCommand_LoadFont:
            {
                rc_load_font* Command = (rc_load_font*)Data;
                font_texture* Dest    = (font_texture*)Command->FontSlot->ImplData;

                read_file_result FontFile = Platform.ReadEntireFile(Command->Filename);
                if(FontFile.Contents)
                {
                    uint8* TextureData = (uint8*)Platform.AllocateMemory(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);

                    stbtt_pack_context Pack;
                    stbtt_PackBegin(&Pack, TextureData, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, 1, 0);
                    stbtt_PackSetOversampling(&Pack, Command->OversampleX, Command->OversampleY);
                    stbtt_PackFontRange(&Pack, (uint8*)FontFile.Contents, 0, Command->Height, 33, 94, Dest->Chars);
                    stbtt_PackEnd(&Pack);

                    glGenTextures(1, &Dest->Handle);
                    glBindTexture(GL_TEXTURE_2D, Dest->Handle);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D,
                                 0,
                                 GL_RED,
                                 FONT_ATLAS_WIDTH,
                                 FONT_ATLAS_HEIGHT,
                                 0,
                                 GL_RED,
                                 GL_UNSIGNED_BYTE,
                                 TextureData);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    Platform.DeallocateMemory(TextureData);
                    Platform.FreeFileMemory(FontFile.Contents);
                }
                else
                {
                    Platform.Log("Unable to read font file!");
                }
            }
            break;

            case RenderCommand_DrawString:
            {
                rc_draw_string* Command = (rc_draw_string*)Data;
                u8* String              = (u8*)Command + sizeof(rc_draw_string);

                font_texture* Font = (font_texture*)Command->FontSlot->ImplData;

                mat4 TranslationMat = Translate(Vec3(Command->Pos.X, Command->Pos.Y, 0));
                mat4 ScaleMat       = Scale(Vec3(Command->Scale, Command->Scale, 1));
                mat4 MVP            = ViewProjectionMatrix * TranslationMat * ScaleMat;

                GLuint vao;
                gl.glGenVertexArrays(1, &vao);
                gl.glBindVertexArray(vao);

                GLuint vbo;
                gl.glGenBuffers(1, &vbo);
                gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);

                gl.glBindBuffer(GL_ARRAY_BUFFER, GLState.MultiVertexBuffer);
                gl.glBufferData(GL_ARRAY_BUFFER, 256 * 6 * 4 * sizeof(float), 0, GL_STREAM_DRAW);

                gl.glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Font->Handle);

                gl.glUseProgram(GLState.TextShaderProgram);
                GLint PosAttrib = gl.glGetAttribLocation(GLState.TextShaderProgram, "position");
                gl.glVertexAttribPointer(PosAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
                gl.glEnableVertexAttribArray(PosAttrib);

                GLint TexCoordAttrib = gl.glGetAttribLocation(GLState.TextShaderProgram, "vertTexCoord");
                gl.glVertexAttribPointer(
                    TexCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
                gl.glEnableVertexAttribArray(TexCoordAttrib);

                gl.glUniformMatrix4fv(GLState.TextShaderMVP, 1, false, (float*)MVP.Elements);
                gl.glUniform4f(
                    GLState.TextShaderColor, Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);

                vec2 Pos  = Vec2(0, 0);
                int Chars = 0;
                for(u8* Current = String; *Current; ++Current)
                {
                    if(*Current > 32 && *Current < 127)
                    {
                        int Index = *Current - 33;
                        stbtt_aligned_quad Quad;
                        stbtt_GetPackedQuad(
                            Font->Chars, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, Index, &Pos.X, &Pos.Y, &Quad, 0);

                        float Vertices[] = {
                            Quad.x0, Quad.y0, Quad.s0, Quad.t0, Quad.x0, Quad.y1,
                            Quad.s0, Quad.t1, Quad.x1, Quad.y1, Quad.s1, Quad.t1,

                            Quad.x0, Quad.y0, Quad.s0, Quad.t0, Quad.x1, Quad.y1,
                            Quad.s1, Quad.t1, Quad.x1, Quad.y0, Quad.s1, Quad.t0,
                        };

                        gl.glBufferSubData(GL_ARRAY_BUFFER, Chars * sizeof(Vertices), sizeof(Vertices), Vertices);
                        ++Chars;
                    }
                    else if(*Current == ' ')
                    {
                        // TODO(kiljacken): We might want to be smarter about this
                        Pos.X += Font->Chars[0].xadvance;
                    }
                }

                glDrawArrays(GL_TRIANGLES, 0, Chars * 6);
                gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
                gl.glBindVertexArray(0);

                gl.glDeleteBuffers(1, &vbo);
                gl.glDeleteVertexArrays(1, &vao);
            }
            break;

            case RenderCommand_SetCameraMatrix:
            {
                rc_set_camera_matrix* Command = (rc_set_camera_matrix*)Data;
                GLState.ViewMatrix            = Command->Matrix;
                ViewProjectionMatrix          = GLState.ProjectionMatrix * GLState.ViewMatrix;
            }
            break;

            default:
            {
                Assert(!"Unhandled render command!");
            };
        }
    }
}
