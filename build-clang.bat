@echo off

REM Create symlink

pushd %~dp0

IF NOT EXIST build\debug mkdir build\debug

fsutil reparsepoint query .\build\debug\data | find "Mount Point" >nul && echo Junction Found. || (
    echo No junction found, creating...
    mklink /J "./build/debug/data" "./data"
    echo Junction made!
)

popd

REM Make sure you have vcvarsall already running. Removed it from here since it was 
REM causing a problem with other versions of visual studio

set CompilerFlags=-Z7 -MTd -nologo  -GR-  -EHa- -Od -Oi -WX -W4
set WarningsDisabled=-Werror -Wno-writable-strings -Wno-invalid-source-encoding -Wno-null-dereference -Wno-missing-braces -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-function
set Macros=-DLUDUS_INTERNAL=1 -DLUDUS_SLOW=1
set LinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib opengl32.lib winmm.lib ole32.lib shlwapi.lib version.lib

pushd build\debug

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB > lock.tmp
clang-cl %CompilerFlags% %WarningsDisabled% %Macros% --target=x86_64-pc-win32-abi %~dp0code\game.cpp -LD /link -PDB:game_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGenerateAudioSamples %LinkerFlags% 
del lock.tmp
echo ----------------------------
clang-cl %CompilerFlags% %WarningsDisabled% %Macros% --target=x86_64-pc-win32-abi %~dp0code\win32\win32_ludus.cpp /link %LinkerFlags% 
popd