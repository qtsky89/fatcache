#!/bin/bash

autoreconf

CFLAGS="-O2" LIBS="-lpthread -laio"

./configure

make clean

make


