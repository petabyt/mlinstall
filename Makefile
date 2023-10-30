# Makefile for Windows XP/7/10, ReactOS, Linux, maybe MacOS
# Compile from Linux only

RM=rm -rf

APP_NAME=mlinstall

# Main core files
FILES=$(addprefix src/,main.o appstore.o drive.o evproc.o gtk.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o data.o enum_dump.o util.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

# Windows and Linux require different impls for the same file
UNIX_FILES=$(FILES) src/drive-unix.o $(addprefix camlib/src/,$(CAMLIB_CORE) libusb.o backend.o)
WIN_FILES=$(FILES) src/drive-win.o $(addprefix camlib/src/,$(CAMLIB_CORE) libwpd.o)

# Some manual header deps
src/gtk.o: src/lang.h

CFLAGS=-Wall -Wpedantic

all: unix-gtk

unix-gtk: LDFLAGS+=$(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0)
unix-gtk: CFLAGS+=$(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0) -Icamlib/src

unix-gtk: $(UNIX_FILES)
	$(CC) $(UNIX_FILES) $(CFLAGS) $(LDFLAGS) -o unix-gtk

style:
	clang-format -style=file -i src/*.c src/*.h gtk.c

# TODO: I don't know how to get GTK or LibUSB for aarch64.
arm64-linux: CC=aarch64-linux-gnu-gcc
arm64-linux: $(UNIX_FILES)
	$(CC) $(UNIX_FILES) $(CFLAGS) $(LDFLAGS) -o arm64-linux

# Clean incompatible stuff, use between compiling for Windows/Linux
clean-out:
	$(RM) -r src/*.o $(APP_NAME) unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb camlib/src/*.o

# Clean everything
clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-cli win64* win32* SD_BACKUP *.dll

#
#  Windows stuff:
#

windows-gtk/win64-gtk-2021:
	unzip windows-gtk/win64-gtk-2021.zip -d windows-gtk/win64-gtk-2021

windows-gtk/win32-gtk-2013:
	unzip windows-gtk/win32-gtk-2013.zip -d windows-gtk/win32-gtk-2013

libwpd_x64.dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/0.1.4/libwpd_64.dll -O libwpd_x64.dll

libwpd_x86.dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/0.1.4/libwpd_32.dll -O libwpd_x86.dll

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

# Main windows targets, will compile a complete directory,
# copy in DLLs, EXE, README, to a directory. Useful if you have virtualbox, or use WSL.
win-gtk: win64-gtk-$(APP_NAME)
win64-gtk-$(APP_NAME): MINGW=x86_64-w64-mingw32
win64-gtk-$(APP_NAME): CC=$(MINGW)-gcc
win64-gtk-$(APP_NAME): CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Icamlib/src -Iwindows-gtk/win64-gtk-2021/win32/include
win64-gtk-$(APP_NAME): win.res windows-gtk/win64-gtk-2021 $(WIN_FILES) libwpd_x64.dll
	-mkdir win64-gtk-$(APP_NAME)
	$(CC) win.res $(WIN_FILES) windows-gtk/win64-gtk-2021/win32/lib/*.dll libwpd_x64.dll $(CFLAGS) -o win64-gtk-$(APP_NAME)/$(APP_NAME).exe
	cp libwpd_x64.dll win64-gtk-$(APP_NAME)/libwpd.dll
	cp windows-gtk/win64-gtk-2021/win32/lib/*.dll win64-gtk-$(APP_NAME)/
	cp assets/README.txt win64-gtk-$(APP_NAME)/

# 32 bit for all the ReactOS nerds to try
win32-gtk: win32-gtk-$(APP_NAME)
win32-gtk-$(APP_NAME): MINGW=i686-w64-mingw32
win32-gtk-$(APP_NAME): CC=$(MINGW)-gcc
win32-gtk-$(APP_NAME): CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -Icamlib/src -Iwindows-gtk/win32-gtk-2013/win32/include
win32-gtk-$(APP_NAME): win.res windows-gtk/win32-gtk-2013 $(WIN_FILES) libwpd_x86.dll
	-mkdir win32-gtk-$(APP_NAME)
	$(CC) win.res $(WIN_FILES) windows-gtk/win32-gtk-2013/win32/lib/*.dll libwpd_x86.dll $(CFLAGS) -o win32-gtk-$(APP_NAME)/$(APP_NAME).exe
	cp libwpd_x86.dll win32-gtk-$(APP_NAME)/libwpd.dll
	cp windows-gtk/win32-gtk-2013/win32/lib/* win32-gtk-$(APP_NAME)/
	cp assets/README.txt win32-gtk-$(APP_NAME)/

# Main C out, will be used by all targets
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

# Release targets:
win64-gtk-$(APP_NAME).zip: win64-gtk-$(APP_NAME)
	zip -r win64-gtk-$(APP_NAME).zip win64-gtk-$(APP_NAME)

win32-gtk-$(APP_NAME).zip: win32-gtk-$(APP_NAME)
	zip -r win32-gtk-$(APP_NAME).zip win32-gtk-$(APP_NAME)

linux64-gtk-$(APP_NAME).AppImage: unix-gtk
	staticx unix-gtk linux64-gtk-$(APP_NAME).AppImage

# Final release for publishing
release:
	make linux64-gtk-$(APP_NAME).AppImage
	make clean-out
	make win64-gtk-$(APP_NAME).zip
	make clean-out
	make win32-gtk-$(APP_NAME).zip

.PHONY: clean clean-out release win32-gtk win64-gtk all style
