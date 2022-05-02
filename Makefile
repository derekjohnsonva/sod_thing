# SOD does not generally require a Makefile to build. Just drop sod.c and its accompanying
# header files on your source tree and you are done.
CC = clang
CFLAGS = -lm -Ofast -Wall -std=c99

sod: sod.c
	$(CC) sod.c samples/license_plate_detection.c -o license_plate_detection -I. $(CFLAGS)