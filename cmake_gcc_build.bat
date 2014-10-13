mkdir build
cd build
del gwTGATest.exe
cmake -G "MinGW Makefiles" ../source/
mingw32-make
gwTGATest.exe
pause