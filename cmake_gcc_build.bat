mkdir build
cd build
del gwTGATest.exe
cmake -G "MinGW Makefiles" ../source/
mingw32-make 2>make.log
gwTGATest.exe
pause