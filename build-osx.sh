#! /bin/bash

if ! type "greadlink" > /dev/null; then
    if ! type "brew" > /dev/null; then
        printf "installing homebrew... you will want it"
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi
    
    printf "greadlink is not installed :/ trying other path\n"
    printf "we are going to try and make shit work!\n"
    brew install coreutils
fi

pushd "$(dirname $(greadlink -f $0))/code" >/dev/null

Warnings="-Wall -Wno-missing-braces -Wno-c++11-compat-deprecated-writable-strings -Wno-null-dereference -Werror -Wno-unused-function -Wno-sign-compare -Wno-write-strings -Wno-unused-variable -Wno-unused-const-variable"
CompilerOptions="-std=c++11 -fno-exceptions -fdiagnostics-color=auto -fno-strict-overflow -DLUDUS_OSX=1"
LinkerOptions="-framework Cocoa -framework QuartzCore -framework OpenGL -framework IOKit -framework AudioUnit"

if [[ "x$1" == "xrelease" ]]; then
	Mode="RELEASE"
	ModeColor="33"
	OutDir="../build/release"
	CompilerOptions="$CompilerOptions -U_FORTIFY_SOURCE -fno-stack-protector -O2 -fno-strict-aliasing -Wl,-s"
else
	Mode="DEBUG"
	ModeColor="32"
	OutDir="../build/debug"
	Macros="$Macros -DLUDUS_INTERNAL=1 -DLUDUS_SLOW=1 "
    CompilerOptions="$CompilerOptions -O0 -g"
	DoSymLink=1
fi

printf "Building in\e[0;$ModeColor m $Mode\e[0m mode.\n"
printf "=======================\n"

Flags="$Macros $CompilerOptions $Warnings"

mkdir -p "$OutDir"
printf "Building libgame.so... \n"
clang++ game.cpp $Flags -o "$OutDir/libgame.dylib" -dynamiclib

printf "Building osx_ludus... \n"
clang++ OSX/osx_ludus.mm $Flags -o "$OutDir/osx_ludus" $LinkerOptions

printf "Building Data & Bundle... \n"

rm -rf "$OutDir/LudusEngine.app"
mkdir -p "$OutDir/LudusEngine.app/Contents/MacOS"
mkdir -p "$OutDir/LudusEngine.app/Contents/Resources"
cp "$OutDir/osx_ludus" "$OutDir/LudusEngine.app/Contents/MacOS/LudusEngine"
cp "$OutDir/libgame.dylib" "$OutDir/LudusEngine.app/Contents/MacOS"

cp -a ../data/. "$OutDir/LudusEngine.app/Contents/Resources"

printf "Build successful. Have a nice day.\n"

popd > /dev/null