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

set CompilerFlags=-FC -Z7 -MTd -nologo -Gm- -GR- -GS -EHa- -Od -Oi -WX -W4
set WarningsDisabled=-wd4201 -wd4100 -wd4505 -wd4189 -wd4127
set Macros=-DLUDUS_INTERNAL=1 -DLUDUS_SLOW=1
set LinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib opengl32.lib winmm.lib ole32.lib shlwapi.lib version.lib

pushd build\debug

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB > lock.tmp
cl %CompilerFlags% %WarningsDisabled% %Macros% ..\..\code\game.cpp -Fmgame.map -LD /link -incremental:no -opt:ref -PDB:game_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGenerateAudioSamples 
del lock.tmp
cl %CompilerFlags% %WarningsDisabled% %Macros% ..\..\code\win32\win32_ludus.cpp -Fmwin32_ludus.map /link %LinkerFlags%

popd