-include config.mak
APP_NAME=mlinstall
HOME?=/home/$(USER)
DOWNLOADS?=$(HOME)/Downloads

CAMLIB_SRC=src/cl_sym/src
#CAMLIB_SRC=camlib/src

APP_CORE=$(addprefix src/,main.o drive.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o canon_adv.o data.o enum_dump.o lib.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

CFLAGS=-Wall -Wpedantic -I$(CAMLIB_SRC) -I../libui-cross/

ifeq ($(TARGET),linux)
$(info Running Linux build)
include linux.mak
else ifeq ($(TARGET),mac)
$(info running Mac build)
include linux.mak
else ifeq ($(TARGET),win)
$(info running Windows build)
include win.mak
else ifeq ($(TARGET),mac)
$(info Running Mac build)
else
TARGET=linux
include linux.mak
$(info Assuming target is $(TARGET))
endif

distclean:
	$(RM) -r src/*.o $(APP_NAME) unix-gtk unix-cli win-gtk win-cli *.o *.out *.exe *.res gtk libusb $(CAMLIB_SRC)/*.o

# Clean everything
clean: distclean
	rm -rf *.zip *.AppImage unix-gtk unix-tui unix-cli win64* win32* SD_BACKUP *.dll linux *.dylib AppDir

.PHONY: clean clean-out release win32-gtk win64-gtk all style
