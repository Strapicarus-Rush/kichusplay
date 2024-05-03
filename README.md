Requeriments, tested on Linux Debian 6.7.12-1 trixie/sid

g++
libglfw3-dev
libsfml-dev
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
libxi-dev
libfreetype6
libfreetype6-dev
libfreetype-dev



*freeglut3-dev **

*not used in code.
**Not recomended, do not use.

compile:(linux) 

g++ -o gamestart.out gamestart.cpp freetype.cpp -lglfw -lGL -lsfml-audio -pthread -I/usr/include/freetype2 -lfreetype 

Run:
./gamestart.out
