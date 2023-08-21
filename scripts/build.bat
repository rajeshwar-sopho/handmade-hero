@echo off

mkdir ..\build
pushd ..\build

cl -Zi ..\src\win32_handmade.cpp User32.lib

popd
 