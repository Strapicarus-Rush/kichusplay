tested on Linux Debian 6.7.12-1 trixie/sid

g++
cmake 
make 
libx11-dev 
libxi-dev 
libgl1-mesa-dev 
libglu1-mesa-dev 
libxrandr-dev 
libxext-dev 
libxcursor-dev 
libxinerama-dev 
libxi-dev
libsdl2-dev
libsdl2-image-dev
libsdl2-ttf-dev


compile:

g++ -o gamestart.out gamestart.cpp player.cpp camera.cpp playlist.cpp  -I/usr/include/SDL2 -lSDL2 -lSDL2_ttf -lGL -lGLEW -lsfml-audio -O2 && ./gamestart.out