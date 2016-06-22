#! /bin/bash

pushd "$(dirname $(readlink -f $0))/code" >/dev/null

# check if rsync is available
if hash rsync 2>/dev/null; then
    CopyCommand="rsync -a"
else
    CopyCommand="cp -r"
fi

Warnings="-Wall -Werror -Wno-unused-function -Wno-sign-compare -Wno-write-strings -Wno-unused-variable -Wno-unused-but-set-variable"
Macros="-DCOMPILER_GCC"
CompilerOptions="-I. -ILinux -std=c++11 -fno-exceptions -fdiagnostics-color=auto -fno-strict-overflow"

if [[ "x$1" == "xrelease" ]]; then
	Mode="RELEASE"
	ModeColor="33"
	OutDir="../build/release"
	CompilerOptions="$CompilerOptions -U_FORTIFY_SOURCE -fno-stack-protector -O2 -fno-strict-aliasing -Wl,-s"
else
	Mode="DEBUG"
	ModeColor="32"
	OutDir="../build/debug"
	Macros="$Macros -DLUDUS_INTERNAL=1 -DLUDUS_SLOW=1"
	CompilerOptions="$CompilerOptions -O0 -g"
	DoSymLink=1
fi

# I couldn't resist doing fancy formatting :P --inso
NumColors="$(tput colors 2>/dev/null)"
if [[ "$?" == "0" && "$NumColors" -gt "8" ]]; then
	Mode="\e[1;${ModeColor}m$Mode\e[00m"
fi

set -e

echo -e "Building in $Mode mode."
echo -e "======================="

Flags="$Macros $CompilerOptions $Warnings"

LinkLibs="-lm -lGL"
LinkLibsPlatform="$LinkLibs -Ldata/libraries/ -lrt -ldl -lopenal -lX11 -Wl,-rpath=\$ORIGIN/libraries:libraries"

mkdir -p "$OutDir"
touch "$OutDir/lock"
echo "Building libgame.so... "
gcc game.cpp $Flags -o "$OutDir/libgame.so" -shared -fPIC $LinkLibs
rm "$OutDir/lock"

echo "Building linux_x11_ludus..."
gcc Linux/linux_x11_ludus.cpp $Flags -o "$OutDir/linux_x11_ludus" $LinkLibsPlatform

echo "Copying libs & data... "
if [[ -n "$DoSymLink" ]]; then
	[[ -d "$OutDir/data" ]] || ln -s ../../data "$OutDir/"
else
	$CopyCommand ../data "$OutDir/"
fi
$CopyCommand ../linux/* "$OutDir"

echo "Build successful. Have a nice day."

popd > /dev/null
