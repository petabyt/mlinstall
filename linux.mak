UNIX_LIBUI_FILES=$(APP_CORE) src/libui.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o transport.o)

vcam:
	cd ../vcam/ && make libusb.so

# -fsanitize=address
ifeq ($(TARGET),mac)
  LDFLAGS+=-L../libs -lui.A -lusb-1.0.0
  CFLAGS+=-I../libs
else
  LDFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0) -lui
  #LDFLAGS+=-L../vcam/ -lusb -Wl,-rpath=../vcam/ -lui
  CFLAGS+=$(shell pkg-config --cflags libusb-1.0)
endif

linux: $(UNIX_LIBUI_FILES)
	$(CC) $(UNIX_LIBUI_FILES) $(CFLAGS) $(LDFLAGS) -o linux

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY: pkg
pkg:
	bash assets/appify -i assets/mlinstall.png -n mlinstall
	install_name_tool -change /opt/local/lib/libusb-1.0.0.dylib "@executable_path/../Frameworks/libusb-1.0.0.dylib" ./mlinstall.app/Contents/MacOS/mlinstall
	install_name_tool -change /usr/local/lib/libui.A.dylib "@executable_path/../Frameworks/libui.A.dylib" ./mlinstall.app/Contents/MacOS/mlinstall

linux64-gtk-mlinstall.StaticAppImage: linux
	staticx --strip linux linux64-gtk-$(APP_NAME).StaticAppImage

mlinstall-x86_64.AppImage: linux
	cp linux mlinstall
	linuxdeploy --appdir=AppDir --executable=mlinstall -d assets/mlinstall.desktop -i assets/mlinstall.png
	appimagetool AppDir
	rm mlinstall
