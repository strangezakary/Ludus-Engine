@echo off

set CompilerFlags=-O2
set WarningsDisabled=-wd4201 -wd4100 -wd4505 -wd4189
set Macros=-DLUDUS_INTERNAL=0 -DLUDUS_SLOW=0
set LinkerFlags=-opt:ref user32.lib gdi32.lib opengl32.lib winmm.lib ole32.lib shlwapi.lib version.lib

IF NOT EXIST build\release\data mkdir build\release\data

echo ----- Copy -----

xcopy .\data .\build\release\data /s /i /e /h /y

echo ----------------

pushd build\release

cl %CompilerFlags% %WarningsDisabled% %Macros% ..\..\code\game.cpp -LD /link  -opt:ref  -EXPORT:GameUpdateAndRender -EXPORT:GameGenerateAudioSamples 
cl %CompilerFlags% %WarningsDisabled% %Macros% ..\..\code\win32\win32_ludus.cpp /link %LinkerFlags%

echo Deleting *.obj *.lib *.exp...
del *.obj *.lib *.exp

popd