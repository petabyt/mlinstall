# Makefile for Windows XP/7/10, ReactOS, Linux, maybe MacOS
# Compile from Linux only
all: unix-gtk
-include config.mak
RM=rm -rf
APP_NAME=mlinstall
HOME?=/home/$(USER)
DOWNLOADS?=$(HOME)/Downloads

CAMLIB_SRC=src/cl_sym/src
#CAMLIB_SRC=camlib/src

# Files for each build
APP_CORE=$(addprefix src/,main.o drive.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o canon_adv.o data.o enum_dump.o lib.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

# Windows and Linux require different impls for the same file
UNIX_GTK_FILES=$(APP_CORE) src/gtk.o src/appstore.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o backend.o)
UNIX_TUI_FILES=$(APP_CORE) src/tui.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o backend.o)

# Some manual header deps
src/gtk.o: src/lang.h

CFLAGS=-Wall -Wpedantic -I$(CAMLIB_SRC)

# Only have GTK libs for this target
unix-gtk: LDFLAGS+=$(shell pkg-config --libs gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0)
unix-gtk: CFLAGS+=$(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags --libs libusb-1.0)
unix-gtk: $(UNIX_GTK_FILES)
	$(CC) $(UNIX_GTK_FILES) $(CFLAGS) $(LDFLAGS) -o unix-gtk

unix-tui: LDFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0)
unix-tui: CFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0)
unix-tui: $(UNIX_TUI_FILES)
	$(CC) $(UNIX_TUI_FILES) $(CFLAGS) $(LDFLAGS) -o unix-tui

style:
	clang-format -style=file -i src/*.c src/*.h gtk.c

# TODO: I don't know how to get GTK or LibUSB for aarch64.
arm64-linux: CC=aarch64-linux-gnu-gcc
arm64-linux: $(UNIX_FILES)
	$(CC) $(UNIX_FILES) $(CFLAGS) $(LDFLAGS) -o arm64-linux

# Clean incompatible stuff, use between compiling for Windows/Linux
clean-out:
	$(RM) -r src/*.o $(APP_NAME) unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb $(CAMLIB_SRC)/*.o

# Clean everything
clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-tui unix-cli win64* win32* SD_BACKUP *.dll

include win.mak

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
