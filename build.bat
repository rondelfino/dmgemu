@echo off
if not exist "build" mkdir "build"

REM GENERAL COMPILER FLAGS
set cflags=         /MTd    &:: Force the debug version of the C Runtime Library (CRT) to statically link
set cflags=%cflags% /GR-    &:: Disable C++ runtime type information
set cflags=%cflags% /EHa-   &:: Disable C++ exception handling

REM DEBUG VARIABLES
set debug=-fdiagnostics-absolute-paths  &:: Show full path
set debug=%debug% /Z7                   &:: Produce debug information (.pdb files)
set debug=%debug% /Od                   &:: No optimizations

REM WARNINGS
set warnings=           /WX-
set warnings=%warnings% /W4

REM IGNORE WARNINGS
set warnings=%warnings% -Wno-unused-parameter
set warnings=%warnings% -Wno-writable-strings


pushd build
clang-cl %cflags% %warnings% %debug% ..\main.cpp ..\sm83.cpp ..\instructions.cpp ..\mmu.cpp
popd
