# Makefile for Windows XP/7/10, ReactOS, Linux, maybe MacOS
# Compile from Linux only
-include config.mak
RM=rm -rf
APP_NAME=mlinstall
HOME?=/home/$(USER)
DOWNLOADS?=$(HOME)/Downloads

# Main core files
FILES=$(addprefix src/,main.o appstore.o drive.o evproc.o gtk.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o data.o enum_dump.o util.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

# Windows and Linux require different impls for the same file
UNIX_FILES=$(FILES) src/drive-unix.o $(addprefix camlib/src/,$(CAMLIB_CORE) libusb.o backend.o)

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
