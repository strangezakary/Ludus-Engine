#!/bin/bash
dir="$(dirname $(readlink -f $0))"
exec $dir/build.sh release
