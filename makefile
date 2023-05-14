# Project: test1
# Makefile created by Dev-C++ 5.11

SRCPATH=.
prefix= .
exec_prefix=${prefix}
libdir=${exec_prefix}/
includedir=${prefix}/
SYS_ARCH=X86_64
SYS=LINUX


.SUFFIXES = .c .o
CPP      = g++
CC       = gcc
WINDRES  = windres.exe

SRC1     = stnc.c

OBJ      = stnc.o

LIBX264 = # libx264.a

BIN1	 = stnc
CXXFLAGS = $(CXXINCS)
CFLAGS   = -Wno-maybe-uninitialized -Wshadow -O3 -ffast-math -m64  -Wall -I. -I$(SRCPATH) -std=gnu99 -D_GNU_SOURCE -mpreferred-stack-boundary=6 -fomit-frame-pointer -fno-tree-vectorize -fvisibility=hidden

all: $(BIN) $(BIN1)
clean:
	rm -rf $(OBJ) $(BIN1)

$(BIN1): $(OBJ)
	$(CC) $(CFLAGS) -g -o $(BIN1) $(OBJ) $(LIBX264) -lpthread -lm #-m64 $(CXXLIBS)


$(OBJ):$(SRC1)
	$(CC) -c $(SRC1) -o $(OBJ) $(CFLAGS)

