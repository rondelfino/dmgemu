@echo off
if not exist "build" mkdir "build"

REM GENERAL COMPILER FLAGS
set cflags=/MTd &:: Force the debug version of the C Runtime Library (CRT) to statically link
set cflags=%cflags% /GR- &:: Disable C++ runtime type information
set cflags=%cflags% /EHa- &:: Disable C++ exception handling

REM DEBUG VARIABLES
set debug=-fdiagnostics-absolute-paths  &:: Show full path
set debug=%debug% /Z7 &:: Produce debug information (.pdb files)
set debug=%debug% /Od &:: No optimizations

REM INCLUDES
set includes="R:\dmg\external\SDL\include"

REM LINKER FLAGS
set ldflags=

REM LIBRARIES
set libs=/LIBPATH:"R:\dmg\external\SDL\build\Release" SDL3.lib

REM WARNINGS
set warnings=/WX-
set warnings=%warnings% /W4

REM IGNORE WARNINGS
set warnings=%warnings% -Wno-unused-parameter
set warnings=%warnings% -Wno-writable-strings

if not exist "build\SDL3.dll" (
    pushd external\SDL

    cmake -S . -B build && cmake --build build --config Release
    copy build\Release\SDL3.dll ..\..\build
    copy build\Release\SDL3.lib ..\..\build

    popd
)

pushd build

set sources=..\src\main.c ..\src\sm83.c ..\src\instructions.c ..\src\memory.c ..\src\idu.c ..\src\gb.c ..\src\timers.c

clang-cl /I%includes% %cflags% %warnings% %debug% %sources% /link %ldflags% %libs%

popd
