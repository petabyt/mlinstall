CFLAGS = 
LDFLAGS = -lusb
STYLE = -style=file -i

default: unix-cli clean
gui-test: unix-gtk unix-clean

unix-clean:
	@rm -rf ptpcam *.orig *.gch *.o *.out ptpcam mlinstall *.exe *.zip *.res linux* ML_*

# Format files to kernel style
style:
	@cd src; clang-format $(STYLE) *.c
	@clang-format $(STYLE) *.c

# Build with GTK, and test
GTKFLAGS = `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0`
unix-gtk:
	$(CC) gtk.c src/*.c $(LDFLAGS) $(CFLAGS) $(GTKFLAGS) -DDEV_ -o mlinstall

# Build with cli, and test
unix-cli:
	$(CC) cli.c src/*.c $(CFLAGS) $(LDFLAGS) -o mlinstall

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

# Desired libusb dll directory
LIBUSB = libusb
LIBUSB_DLL = $(LIBUSB)/bin/amd64/libusb0.dll

# win32 + LIBUSB libs
WIN_CFLAGS = -lws2_32 -lkernel32 -I$(LIBUSB)/include -Igtk/include

# Download Windows DLLs (libusb, gtk)
# Alternative source: https://download.geany.org/contrib/gtk/gtk+-bundle_3.8.2-20131001_win32.zip
# or https://web.archive.org/web/20171023023802if_/http://win32builder.gnome.org/gtk+-bundle_3.10.4-20131202_win64.zip
win-libs:
	@mkdir gtk
	@wget -4 https://github.com/petabyt/windows-gtk/raw/master/win64-gtk-2021.zip
	@unzip win64-gtk-2021.zip -d gtk
	@mv gtk/win32 .; rm -rf gtk; mv win32 gtk
	@wget -4 https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	@unzip libusb-win32-bin-1.2.2.0.zip
	@mv libusb-win32-bin-1.2.2.0 libusb
	@rm *.zip

win-clean:
	@rm -rf gtk libusb-win32-bin-1.2.2.0 *.zip *.exe *.res mlinstall/ libusb lib

win-gtk:
	@$(WINCC)-windres assets/win.rc -O coff -o win.res
	$(WINCC)-gcc gtk.c win.res src/*.c $(WIN_CFLAGS) $(LIBUSB_DLL) gtk/lib/* -o mlinstall.exe

win-gtk-test:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cp $(LIBUSB_DLL) mlinstall/
	@cd gtk/lib/; cp * ../../mlinstall/
	@cp mlinstall.exe mlinstall/
	@echo "Please report bugs at https://github.com/petabyt/mlinstall" > mlinstall/README.txt
	@echo "If you have issues, see https://petabyt.github.io/mlinstall/MANUAL" > mlinstall/README.txt

win-gtk-pack: win-gtk-test
	@zip -r win64-gtk-mlinstall.zip mlinstall

# Compile cli app as 32 bit
# (so I can run it on Windows XP)
win-cli:
	i686-w64-mingw32-gcc cli.c src/*.c $(WIN_CFLAGS) $(LIBUSB)/bin/x86/libusb0_x86.dll -o mlinstall.exe

win-cli-test:
	@rm -rf mlinstall
	@mkdir mlinstall
	@cp $(LIBUSB)/bin/x86/libusb0_x86.dll mlinstall/libusb0.dll
	@cp mlinstall.exe mlinstall/

win-cli-pack: win-cli-test
	@zip -r win32-cli-mlinstall.zip mlinstall
