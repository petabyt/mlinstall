#CC = cc
CFLAGS= 
LDFLAGS=-lusb
STYLE=-style=file -i

LIBFILES = flag.c myusb.c properties.c ptp.c ptpcam.c

GTKFLAGS = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`

all: ptpcam clean
gui: gtkb clean

clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam *.exe

style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

gtkb:
	@cd src; $(CC) ../gtk.c $(LIBFILES) $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -o ../ptpcam
	@./ptpcam

ptpcam:
	@cd src; $(CC) ../main.c $(LIBFILES) -o ../ptpcam $(CFLAGS) $(LDFLAGS)
	@sudo ./ptpcam
