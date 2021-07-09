# Create standalone executable
STATIC = -static -static-libgcc -static-libstdc++

#CC = tcc
CFLAGS = -w
LDFLAGS = -lusb
STYLE = -style=file -i
#LDFLAGS+=$(STATIC)

LIBFILES = myusb.c properties.c ptp.c ptpcam.c

GTKFLAGS = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`

all: mlinstall clean
gui: gtkb clean

clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam mlinstall *.exe *.zip

# Format kernel style
style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

gtkb:
	cd src; $(CC) ../gtk.c flag.c $(LIBFILES) $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -o ../mlinstall
	@./mlinstall

mlinstall:
	@cd src; $(CC) ../main.c flag.c $(LIBFILES) $(CFLAGS) $(LDFLAGS) -o ../mlinstall
	@sudo ./mlinstall

# ------------------------------------------------
# Targets to cross compile for windows, from Linux
# ------------------------------------------------

# Download from https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
# And export top directory to folder "gtk"
# Unzip https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip/download

WINCC = x86_64-w64-mingw32-gcc
LIBUSB = libusb-win32-bin-1.2.2.0

# Desired libusb dll directory
LLIBUSB = ../$(LIBUSB)/bin/amd64/libusb0.dll

# LIBUSB libs
LIB = -lws2_32 -lkernel32 -I$(LIBUSB)/include -I../$(LIBUSB)/include $(LLIBUSB) 

GLIB = -mms-bitfields -I../gtk/include/gtk-3.0 -I../gtk/include/cairo -I../gtk/include/pango-1.0 -I../gtk/include/atk-1.0 -I../gtk/include/cairo -I../gtk/include/pixman-1 -I../gtk/include -I../gtk/include/freetype2 -I../gtk/include/libxml2 -I../gtk/include/freetype2 -I../gtk/include/libpng16 -I../gtk/include/gdk-pixbuf-2.0 -I../gtk/include/libpng16 -I../gtk/include/glib-2.0 -I../gtk/lib/glib-2.0/include -L../gtk/lib -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -lpangocairo-1.0 -lpangowin32-1.0 -lgdi32 -lpango-1.0 -lm -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lglib-2.0
GLIB += ../gtk/bin/libgtk-3-0.dll ../gtk/bin/libgobject-2.0-0.dll

# Download Windows libs (libusb, gtk)
setuplibs:
	@mkdir gtk
	@wget https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
	@unzip gtk+-bundle_3.10.4-20131202_win64.zip -d gtk
	@wget https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	@unzip libusb-win32-bin-1.2.2.0.zip
	@rm *.zip

removelibs:
	@rm -rf gtk libusb-win32-bin-1.2.2.0

windowsgtk:
	cd src; $(WINCC) ../gtk.c flag-win.c $(LIBFILES) $(LIB) $(GLIB) $(CFLAGS) -o ../mlinstall.exe

windows:
	cd src; $(WINCC) ../main.c flag-win.c $(LIBFILES) $(LIB) $(CFLAGS) -o ../mlinstall.exe

windowsgtkpack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LLIBUSB) ../mlinstall/
	@cp gtk/bin/*.dll mlinstall/
	@cp mlinstall.exe mlinstall/
	@zip -r install.zip mlinstall

windowspack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LLIBUSB) ../mlinstall/
	@cp mlinstall.exe mlinstall/
	@zip -r install.zip mlinstall