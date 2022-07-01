# Makefile for Windows XP/7/10, ReactOS, Linux, maybe MacOS
# Compile from Linux only

# Note: Platform specific files will not be
# compiled because of "#ifdef WIN32" guards
FILES=$(patsubst %.c,%.o,$(wildcard src/*.c))

all: unix-gtk

# flags for unix-gtk
unix-gtk: LDFLAGS=-lusb $(shell pkg-config --libs gtk+-3.0)
unix-gtk: CFLAGS=$(shell pkg-config --cflags gtk+-3.0)

# Clean incompatible stuff, use between comiling 
clean-out:
	$(RM) -r src/*.o mlinstall unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb

# Clean everything
clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-cli win64* win32*

unix-gtk: $(FILES) gtk.o
	$(CC) gtk.o $(FILES) $(CFLAGS) $(LDFLAGS) -o unix-gtk

style:
	clang-format -style=file -i src/*.c src/*.h gtk.c

#
#  Windows stuff:
#

# Download GTK libs, GTK_ZIP is sent from parent target
# See https://github.com/petabyt/windows-gtk for more info on this
gtk:
	mkdir gtk
	wget -4 https://github.com/petabyt/windows-gtk/raw/master/$(GTK_ZIP)
	unzip $(GTK_ZIP) -d gtk
	mv gtk/win32 .
	rm -rf gtk
	mv win32 gtk

libusb:
	wget -4 https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	unzip libusb-win32-bin-1.2.2.0.zip
	mv libusb-win32-bin-1.2.2.0 libusb

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

# Main windows targets, will compile a complete directory,
# copy in DLLs, README. Useful for testing in virtualbox and stuff
win-gtk: win64-gtk-mlinstall
win64-gtk-mlinstall: MINGW=x86_64-w64-mingw32
win64-gtk-mlinstall: CC=$(MINGW)-gcc
win64-gtk-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Ilibusb/include -Igtk/include
win64-gtk-mlinstall: GTK_ZIP=win64-gtk-2021.zip
win64-gtk-mlinstall: win.res gtk libusb gtk.o $(FILES)
	-mkdir win64-gtk-mlinstall
	$(CC) win.res gtk.o $(FILES) gtk/lib/* libusb/bin/amd64/libusb0.dll \
	    $(CFLAGS) -o win64-gtk-mlinstall/mlinstall.exe
	cp libusb/bin/amd64/libusb0.dll win64-gtk-mlinstall/
	cd gtk/lib/; cp * ../../win64-gtk-mlinstall/
	cp assets/README.txt win64-gtk-mlinstall/

# 32 bit Windows XP, ReactOS
win32-gtk: win32-gtk-mlinstall
win32-gtk-mlinstall: MINGW=i686-w64-mingw32
win32-gtk-mlinstall: CC=$(MINGW)-gcc
win32-gtk-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Ilibusb/include -Igtk/include
win32-gtk-mlinstall: GTK_ZIP=win32-gtk-2013.zip
win32-gtk-mlinstall: win.res gtk libusb gtk.o $(FILES)
	-mkdir win32-gtk-mlinstall
	$(CC) win.res gtk.o $(FILES) gtk/lib/* libusb/bin/x86/libusb0_x86.dll \
	    $(CFLAGS) -o win32-gtk-mlinstall/mlinstall.exe
	cp libusb/bin/x86/libusb0_x86.dll win32-gtk-mlinstall/libusb0.dll
	cp gtk/lib/* win32-gtk-mlinstall/
	cp assets/README.txt win32-gtk-mlinstall/

# Main C out, will be used by all targets
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

# Release targets:
win64-gtk-mlinstall.zip: win64-gtk-mlinstall
	zip -r win64-gtk-mlinstall.zip win64-gtk-mlinstall

win32-gtk-mlinstall.zip: win32-gtk-mlinstall
	zip -r win32-gtk-mlinstall.zip win32-gtk-mlinstall

linux64-gtk-mlinstall.AppImage: unix-gtk
	staticx unix-gtk linux64-gtk-mlinstall.AppImage

release:
	$(MAKE) linux64-gtk-mlinstall.AppImage clean-out
	$(MAKE) win64-gtk-mlinstall.zip clean-out
	$(MAKE) win32-gtk-mlinstall.zip clean-out

.PHONY: clean clean-out release win32-gtk win64-gtk all style
