#!/bin/bash
pushd $(dirname "$(readlink -f $0)")
find code -type f \( -iname '*.cpp' -o -iname '*.h' \) -exec clang-format -style=file -i {} +
popd
