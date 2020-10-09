#! /bin/bash

cd src
cd bbcb
gcc -Os -I/usr/include/SDL -c sound.c mode2font.c r6502main.c
ar cr libbbcb.a sound.o r6502main.o mode2font.o
cd ..
gcc -Os -o ../chuckie-egg -I/usr/include/SDL -I../ execute.c library.c main.c -Lbbcb -lbbcb -lSDL -lm
