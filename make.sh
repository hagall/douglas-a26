#/bin/bash
g++  $(pkg-config --cflags dbus-1 dbus-glib-1 ) -std=c++0x -c douglas-a26.cpp
g++  $(pkg-config --libs dbus-1 dbus-glib-1) -lboost_regex -lboost_program_options douglas-a26.o -o douglas-a26
