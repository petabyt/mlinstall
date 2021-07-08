CFLAGS=-w
LDFLAGS=-lusb
STYLE=-style=file -i

all: ptpcam clean

clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam ptpcam.exe

style:
	@clang-format $(STYLE) ptp.c
	@clang-format $(STYLE) myusb.c
	@clang-format $(STYLE) properties.c
	@clang-format $(STYLE) ptpcam.c
	@clang-format $(STYLE) flag.c
	@clang-format $(STYLE) main.c
	@clang-format $(STYLE) flag-win.c

gtk:
	@$(CC) gtk.c `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
	@./a.out

ptpcam:
	@$(CC) flag.c main.c myusb.c properties.c ptp.c ptpcam.c -o ptpcam $(CFLAGS) $(LDFLAGS)
	@sudo ./ptpcam
