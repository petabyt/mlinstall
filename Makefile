# This Makefile contains targets for Windows 7/10
# And POSIX systems.

CFLAGS = -O0
LDFLAGS = -lusb

default: unix-gtk

clean: win-clean unix-clean

unix-clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam mlinstall *.exe *.zip *.res linux* ML_*

# Format files to kernel style
STYLE = -style=file -i
style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

GTKFLAGS=$(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --libs gtk+-3.0)
unix-gtk:
	$(CC) gtk.c src/*.c $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -DDEV_ -o mlinstall

linux64-gtk-mlinstall: unix-gtk
	staticx mlinstall linux64-gtk-mlinstall

unix-cli:
	$(CC) cli.c src/*.c $(CFLAGS) $(LDFLAGS) -o mlinstall

linux64-cli-mlinstall: unix-cli
	staticx mlinstall linux64-gtk-mlinstall

release: linux64-gtk-mlinstall \
	linux64-cli-mlinstall \
	win-libs win-gtk win-gtk-pack \
	win-cli win-cli-pack

# ------------------------------------------------
# Rules to cross compile for windows, from Linux
# ------------------------------------------------
# You need to have x86_64-w64-mingw32-gcc.

WINCC = x86_64-w64-mingw32

# Desired libusb dll directory
LIBUSB = libusb
LIBUSB_DLL = $(LIBUSB)/bin/amd64/libusb0.dll

# win32 + LIBUSB libs
WIN_CFLAGS = -lws2_32 -lkernel32 -lurlmon -I$(LIBUSB)/include -Igtk/include

# strip debug symbols, smaller executable
WIN_CFLAGS += -s

# Download Windows DLLs (libusb, gtk)
# Alternative source: https://download.geany.org/contrib/gtk/gtk+-bundle_3.8.2-20131001_win32.zip
# or https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
gtk:
	mkdir gtk
	wget -4 https://github.com/petabyt/windows-gtk/raw/master/win64-gtk-2021.zip
	unzip win64-gtk-2021.zip -d gtk
	mv gtk/win32 .; rm -rf gtk; mv win32 gtk

libusb:
	wget -4 https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	unzip libusb-win32-bin-1.2.2.0.zip
	mv libusb-win32-bin-1.2.2.0 libusb
	rm *.zip

win-libs: gtk libusb

win-clean:
	rm -rf gtk libusb-win32-bin-1.2.2.0 *.zip *.exe *.res mlinstall/ libusb lib

mlinstall:
	rm -rf mlinstall
	mkdir mlinstall

win-gtk: win-libs
	$(WINCC)-windres assets/win.rc -O coff -o win.res
	$(WINCC)-gcc gtk.c win.res src/*.c $(WIN_CFLAGS) $(LIBUSB_DLL) gtk/lib/* -o mlinstall.exe

# Copy over files but don't pack, for virtualbox testing
win-gtk-test: mlinstall
	cp $(LIBUSB_DLL) mlinstall/
	cd gtk/lib/; cp * ../../mlinstall/
	cp mlinstall.exe mlinstall/
	cp assets/README.txt mlinstall/

win-gtk-pack: win-gtk-test
	@zip -r win64-gtk-mlinstall.zip mlinstall

# Compile cli app as 32 bit
# (so I can run it on Windows XP)
win-cli: win-libs
	i686-w64-mingw32-gcc cli.c src/*.c $(WIN_CFLAGS) $(LIBUSB)/bin/x86/libusb0_x86.dll -o mlinstall.exe

win-cli-test: mlinstall
	cp $(LIBUSB)/bin/x86/libusb0_x86.dll mlinstall/libusb0.dll
	cp mlinstall.exe mlinstall/

win32-cli-mlinstall.zip: win-cli-test
	zip -r win32-cli-mlinstall.zip mlinstall
