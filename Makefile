# Makefile for Windows XP/7/10, ReactOS, Linux, maybe MacOS
# Compile from Linux only

# Note: Platform specific files will not be
# compiled because of "#ifdef WIN32" guards
FILES=$(patsubst %.c,%.o,$(wildcard src/*.c))

CL_OBJ_=operations.o packet.o enums.o data.o enum_dump.o util.o canon.o liveview.o bind.o
UNIX_FILES=$(FILES) $(addprefix camlib/src/,$(CL_OBJ_) libusb.o backend.o)
WIN_FILES=$(FILES) $(addprefix camlib/src/,$(CL_OBJ_) libwpd.o)

RM=rm -rf

all: unix-gtk

# flags for unix-gtk
unix-gtk: LDFLAGS=$(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0)
unix-gtk: CFLAGS=$(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0) -Icamlib/src -DVERBOSE

# Clean incompatible stuff, use between comiling 
clean-out:
	$(RM) -r src/*.o mlinstall unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb camlib/src/*.o

# Clean everything
clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-cli win64* win32* SD_BACKUP *.dll

unix-gtk: $(UNIX_FILES) gtk.o
	$(CC) gtk.o $(UNIX_FILES) $(CFLAGS) $(LDFLAGS) -o unix-gtk

style:
	clang-format -style=file -i src/*.c src/*.h gtk.c

#
#  Windows stuff:
#

windows-gtk/win64-gtk-2021:
	unzip windows-gtk/win64-gtk-2021.zip -d windows-gtk/win64-gtk-2021

windows-gtk/win32-gtk-2013:
	unzip windows-gtk/win32-gtk-2013.zip -d windows-gtk/win32-gtk-2013

libwpd_x64.dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/0.1.3/libwpd_64.dll -O libwpd_x64.dll

libwpd_x86.dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/0.1.3/libwpd_32.dll -O libwpd_x86.dll

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

# Main windows targets, will compile a complete directory,
# copy in DLLs, README. Useful for testing in virtualbox and stuff
win-gtk: win64-gtk-mlinstall
win64-gtk-mlinstall: MINGW=x86_64-w64-mingw32
win64-gtk-mlinstall: CC=$(MINGW)-gcc
win64-gtk-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Icamlib/src -Iwindows-gtk/win64-gtk-2021/win32/include
win64-gtk-mlinstall: win.res $(WIN_FILES) libwpd_x64.dll windows-gtk/win64-gtk-2021
	-mkdir win64-gtk-mlinstall
	$(CC) win.res $(WIN_FILES) windows-gtk/win64-gtk-2021/win32/lib/*.dll libwpd_x64.dll $(CFLAGS) -o win64-gtk-mlinstall/mlinstall.exe
	cp libwpd_x64.dll win64-gtk-mlinstall/libwpd.dll
	cp windows-gtk/win64-gtk-2021/win32/lib/*.dll win64-gtk-mlinstall/
	cp assets/README.txt win64-gtk-mlinstall/

# 32 bit Windows XP, ReactOS
win32-gtk: win32-gtk-mlinstall
win32-gtk-mlinstall: MINGW=i686-w64-mingw32
win32-gtk-mlinstall: CC=$(MINGW)-gcc
win32-gtk-mlinstall: CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Icamlib/src -Iwindows-gtk/win32-gtk-2013/win32/include
win32-gtk-mlinstall: win.res $(WIN_FILES) libwpd_x86.dll windows-gtk/win32-gtk-2013
	-mkdir win32-gtk-mlinstall
	$(CC) win.res $(WIN_FILES) windows-gtk/win32-gtk-2013/win32/lib/*.dll libwpd_x86.dll $(CFLAGS) -o win32-gtk-mlinstall/mlinstall.exe
	cp libwpd_x86.dll win32-gtk-mlinstall/libwpd.dll
	cp windows-gtk/win32-gtk-2013/win32/lib/* win32-gtk-mlinstall/
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
	$(MAKE) linux64-gtk-mlinstall.AppImage
	$(MAKE) clean-out
	$(MAKE) win64-gtk-mlinstall.zip
	$(MAKE) clean-out
	$(MAKE) win32-gtk-mlinstall.zip

.PHONY: clean clean-out release win32-gtk win64-gtk all style
