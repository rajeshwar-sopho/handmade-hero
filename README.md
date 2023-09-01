# Homemade Hero

This contains code that I wrote while following Molly Rocket's Homemade Hero Series.

## Description

Hope to learn making games from scratch

## Getting Started

### Notes

#### Day 11

* Platform independence was discussed.
* We are going to separate code into two parts
    1. Platform code that call the game code (gettings stuff to display, sound etc.).
    2. Game code that calls the platform code (getting input).
* By including `handmade.cpp` file in `win32_handmade.cpp` we don't have to specify it to the compiler. Compiler will automatically compile it.