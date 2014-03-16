CXX = g++
CC  = gcc
LD  = ld

#Some of the flags that affect us
STRESS_FLAGS = #-O2 #-fomit-frame-pointer #-O3

BINUTILS_TARGET  = -DTARGET='"x86_64-pc-linux-gnu"'
BINUTILS_SOURCE  = ../binutils-2.18
BINUTILS_LDFLAGS = -L$(BINUTILS_SOURCE)/bfd -L$(BINUTILS_SOURCE)/libiberty -lbfd -liberty
BINUTILS_CFLAGS  = $(BINUTILS_TARGET) -I$(BINUTILS_SOURCE)/bfd -I$(BINUTILS_SOURCE)/include -I$(BINUTILS_SOURCE)/binutils

LIBCTRACE_HEADERS =
LIBCTRACE_FILES   = addr2line.c
LIBCTRACE_OBJ     = $(LIBCTRACE_FILES:%.c=%.o)
LIBCTRACE_CFLAGS  = -g -Wall -fPIC -I. $(BINUTILS_CFLAGS) -save-temps $(STRESS_FLAGS)
LIBCTRACE_LDFLAGS = -shared $(BINUTILS_LDFLAGS) -ldl -lc


all: profile-filter addr2line

addr2line: addr2line.o
	$(CC) -DMAIN_FUNC -o $@ $< $(BINUTILS_CFLAGS) $(BINUTILS_LDFLAGS) 

profile-filter: addr2line.o profile-filter.o addr2line.h
	$(CXX) -g -o $@ addr2line.o profile-filter.o $(BINUTILS_CFLAGS) $(BINUTILS_LDFLAGS)

%.o: %.c $(LIBCTRACE_HEADERS)
	$(CC) $(LIBCTRACE_CFLAGS) -c $< -o $@

%.o: %.cpp $(LIBCTRACE_HEADERS)
	$(CXX) $(LIBCTRACE_CFLAGS) -c $< -o $@

clean:
	rm -f *.o profile-filter addr2line
