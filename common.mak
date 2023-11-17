-include config.mak
RM=rm -rf
APP_NAME=mlinstall
HOME?=/home/$(USER)
DOWNLOADS?=$(HOME)/Downloads

#CAMLIB_SRC=src/cl_sym/src
CAMLIB_SRC=camlib/src

# Files for each build
APP_CORE=$(addprefix src/,main.o drive.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o canon_adv.o data.o enum_dump.o lib.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

CFLAGS=-Wall -Wpedantic -I$(CAMLIB_SRC)

# Clean incompatible stuff, use between compiling for Windows/Linux
clean-out:
	$(RM) -r src/*.o $(APP_NAME) unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb $(CAMLIB_SRC)/*.o

# Clean everything
clean: clean-out
	$(RM) -r *.zip *.AppImage unix-gtk unix-tui unix-cli win64* win32* SD_BACKUP *.dll linux

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
