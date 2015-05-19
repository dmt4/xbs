


CC = gcc

CFLAGS = -O3

LIBS = -lX11 -lm 

LIBPATH = -L/usr/X11R6/lib


all:	xbs.c
	$(CC) $(CFLAGS) -o xbs xbs.c $(LIBPATH) $(LIBS)


clean:
	-rm *.o xbs
