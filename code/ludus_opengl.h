/*
    Creation Date: 28-03-2016
    Creator: Emil Lauridsen
*/

#if !defined(LUDUS_OPENGL_H)

#include "stb_truetype.h"

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_NUM_EXTENSIONS 0x821D
#define GL_TEXTURE0 0x84C0
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_INFO_LOG_LENGTH 0x8B84
#define DEBUG_SEVERITY_HIGH 0x9146
#define DEBUG_SEVERITY_MEDIUM 0x9147
#define DEBUG_SEVERITY_LOW 0x9148
#define DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_DEBUG_OUTPUT 0x92E0

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001

typedef float GLfloat;
typedef int GLsizei;
typedef int GLint;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;
typedef void GLvoid;
typedef unsigned char GLubyte;
typedef unsigned int GLbitfield;
typedef float GLclampf;
typedef void
gl_debug_proc(GLenum source,
              GLenum type,
              GLuint id,
              GLenum severity,
              GLsizei length,
              const GLchar* message,
              const void* userParam);

typedef void
gl_gen_buffers(GLsizei n, GLuint* buffers);
typedef void
gl_bind_buffer(GLenum target, GLuint buffer);
typedef void
gl_buffer_data(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
typedef GLuint
gl_create_shader(GLenum shaderType);
typedef void
gl_shader_source(GLuint shader, GLsizei count, GLchar** string, const GLint* length);
typedef void
gl_compile_shader(GLuint shader);
typedef void
gl_get_shaderiv(GLuint shader, GLenum pname, GLint* params);
typedef void
gl_get_shader_info_log(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef GLuint
gl_create_program(void);
typedef void
gl_attach_shader(GLuint program, GLuint shader);
typedef void
gl_bind_frag_data_location(GLuint program, GLuint colorNumber, const char* name);
typedef void
gl_link_program(GLuint program);
typedef void
gl_use_program(GLuint program);
typedef void
gl_get_program_iv(GLuint program, GLenum pname, GLint* params);
typedef void
gl_get_program_info_log(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef GLint
gl_get_attrib_location(GLuint program, const GLchar* name);
typedef void
gl_vertex_attrib_pointer(
    GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
typedef void
gl_enable_vertex_attrib_array(GLuint index);
typedef void
gl_disable_vertex_attrib_array(GLuint index);
typedef void
gl_gen_vertex_arrays(GLsizei n, GLuint* arrays);
typedef void
gl_bind_vertex_array(GLuint array);
typedef void
gl_draw_arrays(GLenum mode, GLint first, GLsizei count);
typedef GLint
gl_get_uniform_location(GLuint program, const GLchar* name);
typedef void
gl_delete_vertex_arrays(GLsizei n, const GLuint* arrays);
typedef void
gl_delete_buffers(GLsizei n, const GLuint* buffers);
typedef void
gl_delete_shader(GLuint shader);
typedef void
gl_delete_program(GLuint program);
typedef void
gl_detach_shader(GLuint program, GLuint shader);
typedef GLubyte*
gl_get_stringi(GLenum name, GLuint index);
typedef void
gl_uniform_matrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void
gl_buffer_sub_data(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
typedef void
gl_active_texture(GLenum target);
typedef void
gl_uniform1i(GLint location, GLint v0);
typedef void
gl_uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

// NOTE(kiljacken): The following typedefs are extensions
typedef void
gl_debug_message_callback(gl_debug_proc* callback, const void* userParam);
typedef void
gl_debug_message_callback_arb(gl_debug_proc* callback, const void* userParam);

// NOTE(kiljacken): We store gl function pointer in a struct to avoid collision with existing symbols
typedef struct gl_functions
{
    gl_gen_buffers* glGenBuffers;
    gl_bind_buffer* glBindBuffer;
    gl_buffer_data* glBufferData;
    gl_create_shader* glCreateShader;
    gl_shader_source* glShaderSource;
    gl_compile_shader* glCompileShader;
    gl_get_shaderiv* glGetShaderiv;
    gl_get_shader_info_log* glGetShaderInfoLog;
    gl_create_program* glCreateProgram;
    gl_attach_shader* glAttachShader;
    gl_bind_frag_data_location* glBindFragDataLocation;
    gl_link_program* glLinkProgram;
    gl_use_program* glUseProgram;
    gl_get_program_iv* glGetProgramiv;
    gl_get_program_info_log* glGetProgramInfoLog;
    gl_get_attrib_location* glGetAttribLocation;
    gl_vertex_attrib_pointer* glVertexAttribPointer;
    gl_enable_vertex_attrib_array* glEnableVertexAttribArray;
    gl_disable_vertex_attrib_array* glDisableVertexAttribArray;
    gl_gen_vertex_arrays* glGenVertexArrays;
    gl_bind_vertex_array* glBindVertexArray;
    gl_get_uniform_location* glGetUniformLocation;
    gl_delete_vertex_arrays* glDeleteVertexArrays;
    gl_delete_buffers* glDeleteBuffers;
    gl_delete_shader* glDeleteShader;
    gl_delete_program* glDeleteProgram;
    gl_detach_shader* glDetachShader;
    gl_get_stringi* glGetStringi;
    gl_uniform_matrix4fv* glUniformMatrix4fv;
    gl_buffer_sub_data* glBufferSubData;
    gl_active_texture* glActiveTexture;
    gl_uniform1i* glUniform1i;
    gl_uniform4f* glUniform4f;

    gl_debug_message_callback* glDebugMessageCallback;
    gl_debug_message_callback_arb* glDebugMessageCallbackARB;
} gl_functions;

global_variable gl_functions gl;

#if 0
// NOTE(kiljacken): I transcribed these just in case we need them, we probably never will
typedef void gl_uniform1f(GLint location, GLfloat v0);
typedef void gl_uniform2f(GLint location, GLfloat v0, GLfloat v1);
typedef void gl_uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);

typedef void gl_uniform2i(GLint location, GLint v0, GLint v1);
typedef void gl_uniform3i(GLint location, GLint v0, GLint v1, GLint v2);
typedef void gl_uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

typedef void gl_uniform1ui(GLint location, GLuint v0);
typedef void gl_uniform2ui(GLint location, GLuint v0, GLuint v1);
typedef void gl_uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
typedef void gl_uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

typedef void gl_uniform1fv(GLint location, GLsizei count, const GLfloat *value);
typedef void gl_uniform2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void gl_uniform3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void gl_uniform4fv(GLint location, GLsizei count, const GLfloat *value);

typedef void gl_uniform1iv(GLint location, GLsizei count, const GLint *value);
typedef void gl_uniform2iv(GLint location, GLsizei count, const GLint *value);
typedef void gl_uniform3iv(GLint location, GLsizei count, const GLint *value);
typedef void gl_uniform4iv(GLint location, GLsizei count, const GLint *value);

typedef void gl_uniform1uiv(GLint location, GLsizei count, const GLuint *value);
typedef void gl_uniform2uiv(GLint location, GLsizei count, const GLuint *value);
typedef void gl_uniform3uiv(GLint location, GLsizei count, const GLuint *value);
typedef void gl_uniform4uiv(GLint location, GLsizei count, const GLuint *value);

typedef void gl_uniform_matrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix3x4fv(GLint location, GLsizei count,  GLboolean transpose, const GLfloat *value);
typedef void gl_uniform_matrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
#endif

struct
{
    b32 ModernContext;

    char* Vendor;
    char* Renderer;
    char* Version;
    char* ShadingLanguageVersion;

    GLint MajorVersion;
    GLint MinorVersion;

    b32 KHR_debug;
    b32 ARB_debug_output;
} typedef opengl_info;

internal void
LoadOpenGLFunctions(opengl_info* Info);
internal void
OpenGLCheckExtensions(opengl_info* Info);

// NOTE(zak): This probably is not the best idea, and if any one else would like to reimplement this
// that would be great. Right now shader (the struct) returns a ID for the texture a OldFileWriteTime
// for the shader hot reloading and a IsInitialize function which will be used for linking the
// shaders, this will also be for the hot reloading of shaders.
struct
{
    GLuint ID;
    s64 OldFileWriteTime;
    b32 IsInitialized;
} typedef shader;

internal shader
OpenGLLoadShader(char* FilePath, uint32 ShaderType);
internal GLuint
OpenGLLinkShader(GLuint ShaderOne, GLuint ShaderTwo);

#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 512

typedef struct font_texture
{
    stbtt_packedchar Chars[94];
    GLuint Handle;
} font_texture;

typedef struct gl_state
{
    mat4 ProjectionMatrix;
    mat4 ViewMatrix;
    GLuint AlphaZeroTexture;
    GLuint WhiteTexture;

    shader SimpleVertexShader;
    shader SimpleFragmentShader;
    GLuint SimpleShaderProgram;
    GLint SimpleShaderMVP;
    GLint SimpleShaderColor;

    shader MultiVertexShader;
    shader MultiFragmentShader;
    GLuint MultiShaderProgram;
    GLint MultiShaderMVP;
    GLint MultiShaderColor;
    GLint MultiShaderTexture0;
    GLint MultiShaderTexture1;
    GLint MultiShaderTexture2;
    GLint MultiShaderTexture3;

    shader TextVertexShader;
    shader TextFragmentShader;
    GLuint TextShaderProgram;
    GLint TextShaderMVP;
    GLint TextShaderColor;

    GLuint SimpleVertexArray;
    GLuint SimpleVertexBuffer;

    GLuint OutlineVertexArray;
    GLuint OutlineVertexBuffer;

    GLuint LineVertexArray;
    GLuint LineVertexBuffer;

    GLuint MultiVertexArray;
    GLuint MultiVertexBuffer;
} gl_state;

global_variable gl_state GLState = {0};


#define LUDUS_OPENGL_H
#endif
