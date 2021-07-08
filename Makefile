CC=tcc
CFLAGS=-w
LDFLAGS=-lusb
STYLE=-style=file -i
LIBFILES=flag.c myusb.c properties.c ptp.c ptpcam.c

all: ptpcam clean
gtk: gtkb clean

clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam *.exe

style:
	@clang-format $(STYLE) ptp.c
	@clang-format $(STYLE) myusb.c
	@clang-format $(STYLE) properties.c
	@clang-format $(STYLE) ptpcam.c
	@clang-format $(STYLE) flag.c
	@clang-format $(STYLE) main.c
	@clang-format $(STYLE) flag-win.c
	@clang-format $(STYLE) gtk.c

gtkb:
	@$(CC) gtk.c $(LIBFILES) $(LDFLAGS) $(CFLAGS) -o ptpcam `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
	@./ptpcam

ptpcam:
	@$(CC) main.c $(LIBFILES) $(LIBFILES) -o ptpcam $(CFLAGS) $(LDFLAGS)
	@sudo ./ptpcam
