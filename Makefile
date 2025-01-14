include cross.mk
-include config.mak
APP_NAME := mlinstall

CAMLIB_SRC ?= camlib/src

APP_CORE := $(addprefix src/,main.o drive.o model.o ptp.o data.o)
CAMLIB_CORE := transport.o operations.o packet.o enums.o data.o enum_dump.o lib.o canon.o liveview.o bind.o ml.o conv.o generic.o canon_adv.o no_ip.o

CFLAGS := -Wall -Wpedantic -I$(CAMLIB_SRC) -I../libui-cross/ -O2 -g

UNIX_FILES := $(APP_CORE) src/libui.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o)

ifeq ($(TARGET),l) # ++++++++++++++++
CFLAGS += $(shell $(PKG_CONFIG) --cflags libusb-1.0)
LDFLAGS += $(shell $(PKG_CONFIG) --libs libusb-1.0) -lui -lpthread
FILES := $(call convert_target,$(UNIX_FILES))

linux.out: $(FILES)
	$(CC) $(FILES) $(CFLAGS) $(LDFLAGS) -o linux.out

install: linux.out
	cp linux.out /bin/mlinstall

APPIMAGE_VARS := UPDATE_INFORMATION="gh-releases-zsync|petabyt|mlinstall|latest|mlinstall-x86_64.AppImage"
mlinstall-x86_64.AppImage: linux.out
	$(APPIMAGE_VARS) linuxdeploy --appdir=AppDir --executable=linux.out -d assets/mlinstall.desktop -i assets/mlinstall.png
	appimagetool AppDir

APPIMAGE_VARS := UPDATE_INFORMATION="gh-releases-zsync|petabyt|mlinstall|latest|mlinstall-x86_64.AppImage"
mlinstall-aarch64.AppImage: linux.out
	$(APPIMAGE_VARS) linuxdeploy --appdir=AppDir --executable=linux.out -d assets/mlinstall.desktop -i assets/mlinstall.png
	appimagetool AppDir

else ifeq ($(TARGET),m) # ++++++++++++
LDFLAGS += -lui -lusb-1.0.0
CFLAGS += -I/usr/local/include/libusb-1.0
FILES := $(call convert_target,$(UNIX_FILES))

mac.out: $(FILES)
	$(CC) $(FILES) $(CFLAGS) $(LDFLAGS) -o mac.out

.PHONY: pkg
pkg: mac.out
	bash assets/appify.sh -i assets/mlinstall.png -s mac.out -n mlinstall
	mkdir mlinstall.app/Contents/Frameworks
	cp /opt/local/lib/libusb-1.0.0.dylib mlinstall.app/Contents/Frameworks
	cp /usr/local/lib/libui.dylib mlinstall.app/Contents/Frameworks
	install_name_tool -change /usr/local/opt/libusb/lib/libusb-1.0.0.dylib "@executable_path/../Frameworks/libusb-1.0.0.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
	install_name_tool -change libui.dylib "@executable_path/../Frameworks/libui.dylib" ./mlinstall.app/Contents/MacOS/mlinstall

else ifeq ($(TARGET),w) # +++++++++++++++

WIN_FILES := $(APP_CORE) src/libui.o src/drive-win.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libwpd.o)
WIN_FILES := $(call convert_target,$(WIN_FILES))

LIBS := -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc
LIBS += -lstdc++ -lgcc -static -lpthread -lssp -lurlmon -luuid

# Remove cmd window from startup
LIBS+=-Wl,-subsystem,windows

windows: mlinstall.exe

mlinstall.exe: $(WIN_FILES) assets/win.res $(LIBWPD_A) $(LIBUI_A)
	$(CC) $(WIN_FILES) $(LIBUI_A) $(LIBWPD_A) assets/win.res $(LIBS) -s -o mlinstall.exe

mlinstall_x86_64.exe: mlinstall.exe
	cp mlinstall.exe mlinstall_x86_64.exe

else
$(info Unknown target $(TARGET))
endif # +++++++++++++

-include src/*.d camlib/src/*.d

%.$(TARGET).o: %.c
	$(CC) -MMD -c $< $(CFLAGS) -o $@

%.$(TARGET).o: %.S
	$(CC) -c $< $(CFLAGS) -o $@

distclean:
	$(RM) -r src/*.o camlib/src/*.o src/*.d camlib/src/*.d *.out *.exe assets/*.res libusb $(CAMLIB_SRC)/*.o

clean: distclean
	rm -rf *.zip *.AppImage win64* win32* SD_BACKUP *.dll linux *.dylib AppDir *.tar.gz *.app

release:
	make TARGET=w mlinstall_x86_64.exe
	make TARGET=l mlinstall-x86_64.AppImage
	darling shell -c "make TARGET=m pkg" && tar -czf mlinstall-mac-x86_64.app.tar.gz mlinstall.app

.PHONY: clean clean-out release win32-gtk win64-gtk all style
