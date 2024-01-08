FILES=$(APP_CORE) src/libui.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o transport.o)
FILES:=$(call convert_target,$(FILES))

$(info $(FILES))

vcam:
	cd ../vcam/ && make libusb.so

# -fsanitize=address
ifeq ($(TARGET),m)
LDFLAGS+=-L../libs -lui -lusb-1.0.0
CFLAGS+=-I../libs -I/usr/local/include/libusb-1.0
else
ifdef VCAM
LDFLAGS+=-L../vcam/ -lusb -Wl,-rpath=../vcam/ -lui
else
LDFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0) -lui
endif
CFLAGS+=$(shell pkg-config --cflags libusb-1.0)
endif

linux: $(FILES)
	$(CC) $(FILES) $(CFLAGS) $(LDFLAGS) -o linux

.PHONY: pkg
pkg:
	bash assets/appify -i assets/mlinstall.png -n mlinstall
	mkdir mlinstall.app/Contents/Frameworks
	cp /opt/local/lib/libusb-1.0.0.dylib mlinstall.app/Contents/Frameworks
	cp /usr/local/lib/libui.dylib mlinstall.app/Contents/Frameworks
	install_name_tool -change /opt/local/lib/libusb-1.0.0.dylib "@executable_path/../Frameworks/libusb-1.0.0.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
	#install_name_tool -change /usr/local/lib/libui.dylib "@executable_path/../Frameworks/libui.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
	install_name_tool -change libui.dylib "@executable_path/../Frameworks/libui.dylib" ./mlinstall.app/Contents/MacOS/mlinstall

mlinstall-x86_64.AppImage: linux
	cp linux mlinstall
	linuxdeploy --appdir=AppDir --executable=mlinstall -d assets/mlinstall.desktop -i assets/mlinstall.png
	appimagetool AppDir
	rm mlinstall
