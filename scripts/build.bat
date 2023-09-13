@echo off

mkdir ..\build
pushd ..\build

@REM -FAsc - jumps to the line with error
@REM -Wall - use this for optimization
@REM -O2 - for optimization
@REM -D - this is for compiler time pound defines
cl -D HANDMADE_INTERNAL=1 -D HANDMADE_SLOW=1 -D HANDMADE_WIN32=1 -Zi ..\src\win32_handmade.cpp User32.lib Gdi32.lib

popd
 
