@echo off

mkdir ..\build
pushd ..\build

# -FAsc - jumps to the line with error
cl -FAsc -Zi ..\src\win32_handmade.cpp User32.lib Gdi32.lib

popd
 