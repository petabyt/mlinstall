CFLAGS = 
LDFLAGS = -lusb
STYLE = -style=file -i

default: unix-cli clean
gui-test: unix-gtk unix-clean

unix-clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam mlinstall *.exe *.zip *.res linux64-cli-mlinstall linux64-gtk-mlinstall

# Format files to kernel style
style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

# Build with GTK, and test
GTKFLAGS = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
unix-gtk:
	$(CC) gtk.c src/*.c $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -o mlinstall
	@./mlinstall

# Build with cli, and test
unix-cli:
	@$(CC) cli.c src/*.c $(CFLAGS) $(LDFLAGS) -o mlinstall
	@sudo ./mlinstall

# Use staticx to convert dynamic to static executable
# (pip3 install staticx)
unix-static:
	@staticx mlinstall mlinstall

# TODO: No recursive make?
release:
	@make unix-gtk
	@staticx mlinstall linux64-gtk-mlinstall
	@make unix-cli
	@staticx mlinstall linux64-cli-mlinstall

	@make win-libs
	@make win-gtk
	make win-gtk-pack
	@make win-cli
	make win-cli-pack

# ------------------------------------------------
# Targets to cross compile for windows, from Linux
# ------------------------------------------------
# You need to have x86_64-w64-mingw32-gcc.

WINCC = x86_64-w64-mingw32

# DLLs required by GTK
WIN_GTK_DLL := libglib-2.0-0.dll libintl-8.dll \
libiconv-2.dll libgobject-2.0-0.dll \
libffi-6.dll libgtk-3-0.dll \
libgdk-3-0.dll libcairo-gobject-2.dll \
libcairo-2.dll libfontconfig-1.dll \
libfreetype-6.dll libpng16-16.dll \
zlib1.dll libxml2-2.dll liblzma-5.dll \
libpixman-1-0.dll libgdk_pixbuf-2.0-0.dll \
libgio-2.0-0.dll libgmodule-2.0-0.dll \
libpango-1.0-0.dll libpangocairo-1.0-0.dll \
libpangowin32-1.0-0.dll libatk-1.0-0.dll

# Add include directories
GTK_FLAG = -mms-bitfields -I../gtk/include/gtk-3.0 -I../gtk/include/cairo -I../gtk/include/pango-1.0 -I../gtk/include/atk-1.0
GTK_FLAG += -I../gtk/include/cairo -I../gtk/include/pixman-1 -I../gtk/include -I../gtk/include/freetype2 -I../gtk/include/libxml2
GTK_FLAG += -I../gtk/include/freetype2 -I../gtk/include/libpng16 -I../gtk/include/gdk-pixbuf-2.0 -I../gtk/include/libpng16 -I../gtk/include/glib-2.0
GTK_FLAG += -I../gtk/lib/glib-2.0/include

# Use all DLLs (doesn't actually link them in)
GTK_FLAG += ../gtk/bin/*.dll

# Desired libusb dll directory
LIBUSB = libusb-win32-bin-1.2.2.0
LIBUSB_DLL = ../$(LIBUSB)/bin/amd64/libusb0.dll

# win32 + LIBUSB libs
LIBUSB_FLAG = -lws2_32 -lkernel32 -I$(LIBUSB)/include -I../$(LIBUSB)/include $(LIBUSB_DLL) 

# Download Windows libs (libusb, gtk)
# Alternative source: https://download.geany.org/contrib/gtk/gtk+-bundle_3.8.2-20131001_win32.zip
win-libs:
	@mkdir gtk
	@wget -4 https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
	@unzip gtk+-bundle_3.10.4-20131202_win64.zip -d gtk
	@wget -4 https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	@unzip libusb-win32-bin-1.2.2.0.zip
	@rm *.zip

win-clean:
	@rm -rf gtk libusb-win32-bin-1.2.2.0 *.zip *.exe *.res mlinstall/

win-gtk:
	@$(WINCC)-windres assets/win.rc -O coff -o win.res
	cd src; $(WINCC)-gcc ../gtk.c ../win.res *.c $(LIBUSB_FLAG) $(GTK_FLAG) $(CFLAGS) -o ../mlinstall.exe

win-cli:
	cd src; $(WINCC)-gcc ../cli.c *.c $(LIBUSB_FLAG) $(CFLAGS) -o ../mlinstall.exe

win-gtk-pack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LIBUSB_DLL) ../mlinstall/
	@cd gtk/bin/; cp -t ../../mlinstall/ $(WIN_GTK_DLL)
	@cp mlinstall.exe mlinstall/
	@echo "Please report bugs at https://github.com/petabyt/mlinstall" > mlinstall/README.txt
	@echo "If you have issues, see https://petabyt.github.io/mlinstall/MANUAL" > mlinstall/README.txt
	@zip -r win64-gtk-mlinstall.zip mlinstall

win-cli-pack:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cd src; cp $(LIBUSB_DLL) ../mlinstall/
	@cp mlinstall.exe mlinstall/
	@zip -r win64-cli-mlinstall.zip mlinstall
