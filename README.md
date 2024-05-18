tested on Linux Debian 6.7.12-1 trixie/sid

g++
SDL2
libsdl2-dev
libsdl2-ttf-dev
cmake 
make 
libglm-dev
libx11-dev
libxi-dev 
libgl1-mesa-dev 
libglu1-mesa-dev 
libxrandr-dev 
libxext-dev 
libxcursor-dev 
libxinerama-dev 



compile:

g++ -o gamestart.out gamestart.cpp playlist.cpp -I/usr/include/SDL2 -lSDL2 -lSDL2_ttf -lGL -lGLEW -lsfml-audio && ./gamestart.out