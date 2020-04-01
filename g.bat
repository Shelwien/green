@echo off

del *.exe

set incs=-DNDEBUG -DSTRICT -DNDEBUG -DWIN32 -D_WIN32 -D_MSC_VER -D_aligned_malloc=_aligned_malloc

set opts=-fwhole-program -fstrict-aliasing -fomit-frame-pointer -ffast-math -fpermissive -fno-rtti -fno-stack-protector -fno-stack-check -fno-check-new -fno-exceptions ^
-flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

rem -fprofile-use -fprofile-correction 

set gcc=C:\MinGW820x\bin\g++.exe -march=k8
set path=%gcc%\..\

del *.exe *.o

%gcc% -s -O3 -std=gnu++11 -O9 %incs% %opts% -static green.cpp -o green.exe
rem -o rz.exe

