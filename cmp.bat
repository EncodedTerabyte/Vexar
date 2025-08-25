@echo off

llc -filetype=obj input.ll -o input.o
gcc input.o -lmingw32 -lgcc -static-libgcc -o myprogram

call myprogram