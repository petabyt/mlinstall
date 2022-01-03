# Makefile for Windows XP/7/10, Linux, maybe MacOS
# Compile from Linux only

# Platform specific files will not be
# compiled because of "#ifdef WIN32" guards
FILES=$(patsubst %.c,%.o,$(wildcard src/*.c))

all: unix-gtk

# flags for unix-gtk
unix-gtk: LDFLAGS=-lusb $(shell pkg-config --libs gtk+-3.0)
unix-gtk: CFLAGS=$(shell pkg-config --cflags gtk+-3.0)

unix-cli: LDFLAGS=-lusb

# Clean things that could cause incompatibilities
clean-out:
	$(RM) -r src/*.o mlinstall unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res

clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-cli win64-gtk-mlinstall win32-cli-mlinstall gtk libusb

unix-gtk: $(FILES) gtk.o
	$(CC) gtk.o $(FILES) $(CFLAGS) $(LDFLAGS) -o unix-gtk

unix-cli: $(FILES) cli.o
	$(CC) cli.o $(FILES) $(CFLAGS) $(LDFLAGS) -o unix-cli

# ----------------
#  Windows stuff:
# ----------------

# Download Windows DLLs (libusb, gtk)
gtk:
	mkdir gtk
	wget -4 https://github.com/petabyt/windows-gtk/raw/master/win64-gtk-2021.zip
	unzip win64-gtk-2021.zip -d gtk
	mv gtk/win32 .
	rm -rf gtk
	mv win32 gtk

libusb:
	wget -4 https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	unzip libusb-win32-bin-1.2.2.0.zip
	mv libusb-win32-bin-1.2.2.0 libusb
	rm *.zip

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

# Main windows targets, will compile a complete directory,
# copy in DLLs, README. Useful for testing in virtualbox and stuff

win64-gtk-mlinstall: MINGW=x86_64-w64-mingw32
win64-gtk-mlinstall: CC=$(MINGW)-gcc
win64-gtk-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Ilibusb/include -Igtk/include

win-gtk: win64-gtk-mlinstall
win64-gtk-mlinstall: win.res gtk libusb gtk.o $(FILES)
	mkdir win64-gtk-mlinstall
	$(CC) win.res gtk.o $(FILES) gtk/lib/* libusb/bin/amd64/libusb0.dll \
	    $(CFLAGS) -o win64-gtk-mlinstall/mlinstall.exe
	cp libusb/bin/amd64/libusb0.dll win64-gtk-mlinstall/
	cd gtk/lib/; cp * ../../win64-gtk-mlinstall/
	cp assets/README.txt win64-gtk-mlinstall/

win32-cli-mlinstall: MINGW=i686-w64-mingw32
win32-cli-mlinstall: CC=$(MINGW)-gcc
win32-cli-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Ilibusb/include

win-cli: win32-cli-mlinstall
win32-cli-mlinstall: win.res libusb cli.o $(FILES)
	mkdir win32-cli-mlinstall
	$(CC) win.res cli.o $(FILES) libusb/bin/x86/libusb0.dll $(CFLAGS) -o win32-cli-mlinstall/mlinstall.exe
	cp $(LIBUSB)/bin/x86/libusb0_x86.dll win32-cli-mlinstall/libusb0.dll
	cp assets/README.txt win32-cli-mlinstall/

# This at end so that it isn't immediately evaluated
# Note that GCC is used instead of CC since for windows
# It's set to only x86_64-w64-mingw32
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

# Final release stuff:

win32-cli-mlinstall.zip: win64-gtk-mlinstall
	zip -r win32-cli-mlinstall.zip win64-gtk-mlinstall

win64-gtk-mlinstall.zip: win64-gtk-mlinstall
	zip -r win64-gtk-mlinstall.zip win64-gtk-mlinstall

linux64-gtk-mlinstall.AppImage: unix-gtk
	staticx unix-gtk linux64-gtk-mlinstall.AppImage

linux64-cli-mlinstall.AppImage: unix-cli
	staticx unix-cli linux64-gtk-mlinstall.AppImage
