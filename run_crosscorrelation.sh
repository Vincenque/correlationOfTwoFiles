#!/bin/sh
clear
rm -rf build
mkdir build
cd build
qmake ../src/crosscorrelation.pro -o Makefile
make
cd ..
./build/crosscorrelation
