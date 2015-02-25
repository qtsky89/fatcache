#!/bin/bash

autoreconf -fvi

CFLAGS="-O2" LIBS="-lpthread -laio" ./configure

make clean

make


