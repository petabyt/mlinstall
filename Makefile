-include config.mak
APP_NAME=mlinstall
HOME?=/home/$(USER)
DOWNLOADS?=$(HOME)/Downloads

CAMLIB_SRC?=camlib/src

ifndef TARGET
$(warning TARGET not defined, assuming Linux)
TARGET=l
endif

convert_target=$(patsubst %.o,%.$(TARGET).o,$1)

APP_CORE=$(addprefix src/,main.o drive.o installer.o model.o platform.o ptp.o data.o)
CAMLIB_CORE=operations.o packet.o enums.o canon_adv.o data.o enum_dump.o lib.o canon.o ml.o liveview.o bind.o generic.o no_ip.o conv.o

CFLAGS=-Wall -Wpedantic -I$(CAMLIB_SRC) -I../libui-cross/

ifeq ($(TARGET),l)
$(info Running Linux build)
include linux.mak
else ifeq ($(TARGET),m)
$(info running Mac build)
include linux.mak
else ifeq ($(TARGET),w)
$(info running Windows build)
include win.mak
else
$(info Unknown target $(TARGET))
endif

-include src/*.d camlib/src/*.d

%.$(TARGET).o: %.c
	$(CC) -MMD -c $< $(CFLAGS) -o $@

%.$(TARGET).o: %.S
	$(CC) -c $< $(CFLAGS) -o $@

distclean:
	$(RM) -r src/*.o camlib/src/*.o src/*.d camlib/src/*.d *.out *.exe *.res libusb $(CAMLIB_SRC)/*.o

clean: distclean
	rm -rf *.zip *.AppImage win64* win32* SD_BACKUP *.dll linux *.dylib AppDir *.tar.gz *.app

release:
	make TARGET=w mlinstall_x86_64.exe

.PHONY: clean clean-out release win32-gtk win64-gtk all style
