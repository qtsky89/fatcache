#!/bin/bash

autoreconf -fvi

CFLAGS="-O2" LIBS="-lpthread -laio" ./configure #--enable-debug=full

make clean

make


