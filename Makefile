CC = gcc
CFLAGS = -O3
LIBS = -lX11 -lXt -lm

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
# XQuartz https://xquartz.macosforge.org/trac/wiki/DeveloperInfo
  INCLUDE = -I/opt/X11/include
  LIBPATH = -L/opt/X11/lib
endif


all:	xbs.c subs.h grsubs.h hardcopy.h intc.c
	$(CC) $(CFLAGS) $(INCLUDE) -o xbs xbs.c $(LIBPATH) $(LIBS)


clean:
	-rm *.o xbs
