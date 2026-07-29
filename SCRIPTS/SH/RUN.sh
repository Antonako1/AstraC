#!/bin/sh
cd "$(dirname "$0")/../.." || exit 1
# build/Release/AstraC asm ./AS/MAIN.AS verbose warn err verbose
# build/Release/AstraC comp ./AC/MAIN.AC verbose warn err verbose
build/Release/AstraC comp ./AC/TEST.AC verbose warn err verbose
