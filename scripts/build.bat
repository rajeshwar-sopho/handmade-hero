@echo off

mkdir ..\build
pushd ..\build

@REM -FAsc - jumps to the line with error
@REM -Wall - use this for optimization
@REM -o2 - for optimization
cl -O2 -Zi ..\src\win32_handmade.cpp User32.lib Gdi32.lib

popd
 