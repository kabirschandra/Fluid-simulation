#!/bin/bash

clear

mkdir -p ./bin

clang -O3 \
    ./src/*.c \
    $(pkg-config --cflags --libs sdl3) \
    -lm \
    -o ./bin/main_bin

if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

./bin/main_bin

