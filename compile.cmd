@echo off
cd src/
cl /EHsc /W4 /O2 music.cpp midi.cpp && echo RUNNING && music