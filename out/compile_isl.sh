#!/bin/bash
if [ $# -eq 1 ]
then
    echo "Compiling $1"
    g++ -std=c++11 -g -o ${1%.*} $1 -I../include -L../libs -lisl
else
    echo "Compiling test.cpp"
    g++ -std=c++11 -g -o test test.cpp -I../include -L../libs/ -lisl
fi
