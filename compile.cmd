@echo off
cd src/
cl /EHsc /O2 music.cpp midi.cpp && echo RUNNING && music
cd ../