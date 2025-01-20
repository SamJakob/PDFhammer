#!/usr/bin/env sh

# Resolve the directory of the script
DIR=$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )
cd "$DIR"
 
if [ ! -f "./out/Release/obj/libpdfium.a" ]; then
	echo "[FAILED] Has the release been built?"
	exit 1
fi

# (Re-)create the staging/ directory.
[ -d "./out/Release/staging" ] && rm -rf ./out/Release/staging
mkdir -p out/Release/staging
mkdir out/Release/staging/libs

set -eux
cp -rp ./public ./out/Release/staging/include
cp -rp ./out/Release/obj/libpdfium.a ./out/Release/staging/libs/libpdfium.a
 