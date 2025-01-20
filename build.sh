#!/usr/bin/env sh

# Resolve the directory of the script
DIR=$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )
cd "$DIR"

set -eux

if [ ! -d out/Release ]; then
    # Create a release directory.
    mkdir -p out/Release
fi

if [ ! -f out/Release/args.gn ]; then
    # Initialize it with an empty set of arguments.
    gn gen out/Release
else
    ninja -C out/Release -t clean
fi

# Build the library.
ninja -C out/Release
