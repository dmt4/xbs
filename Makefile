


CC = gcc

CFLAGS = -O3

LIBS = -lX11 -lm 

# Checking LIBPATH
#. (OS detecting makefile)[http://stackoverflow.com/questions/714100/os-detecting-makefile]
UNAME_S := $(shell uname -s)
# Darwin x86_64, with XQuartz(X11) libraries
#. (XQuartz)[https://xquartz.macosforge.org/trac/wiki/DeveloperInfo]
ifeq ($(UNAME_S),Darwin)
  LIBPATH = -L/opt/X11/lib
else
  LIBPATH =
endif


all:	xbs.c
	$(CC) $(CFLAGS) -o xbs xbs.c $(LIBPATH) $(LIBS)


clean:
	-rm *.o xbs
