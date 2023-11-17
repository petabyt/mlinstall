UNIX_LIBUI_FILES=$(APP_CORE) src/libui.o src/drive-unix.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libusb.o transport.o)

LDFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0) -lui
CFLAGS+=$(shell pkg-config --cflags --libs libusb-1.0)
linux: $(UNIX_LIBUI_FILES)
	$(CC) $(UNIX_LIBUI_FILES) $(CFLAGS) $(LDFLAGS) -o linux

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@
