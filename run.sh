clear
clear
clear
mkdir -p ./bin
clang ./src/fluid.c $(pkg-config --cflags --libs sdl3) -fsanitize=address -o ./bin/main_bin
./bin/main_bin


#UP TO HERE:

https://chatgpt.com/c/6ad06845-8d0c-83ec-8a52-796b56bf49ec