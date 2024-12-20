#!/bin/sh

CC=gcc
CFLAGS="-std=c11 -O1 -Wall -Werror -I libraries/libspng-0.7.4"


$CC $CFLAGS libraries/libspng-0.7.4/spng.c source/*.c -lm -lz -o Sneptile
