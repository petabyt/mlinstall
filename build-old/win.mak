# Makefile stuff for windows, included by Makefile

WIN_FILES=$(APP_CORE) src/gtk.o src/appstore.o src/drive-win.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libwpd.o)

windows-gtk/win64-gtk-2021:
	unzip windows-gtk/win64-gtk-2021.zip -d windows-gtk/win64-gtk-2021

windows-gtk/win32-gtk-2013:
	unzip windows-gtk/win32-gtk-2013.zip -d windows-gtk/win32-gtk-2013

LIBWPD_VER=0.1.4

$(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/$(LIBWPD_VER)/libwpd_64.dll -O $(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll

$(DOWNLOADS)/libwpd_x86-$(LIBWPD_VER).dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/$(LIBWPD_VER)/libwpd_32.dll -O $(DOWNLOADS)/libwpd_x86-$(LIBWPD_VER).dll

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

# Main windows targets, will compile a complete directory,
# copy in DLLs, EXE, README, to a directory. Useful if you have virtualbox, or use WSL.
win-gtk: win64-gtk-$(APP_NAME)
win64-gtk-$(APP_NAME): MINGW=x86_64-w64-mingw32
win64-gtk-$(APP_NAME): CC=$(MINGW)-gcc
win64-gtk-$(APP_NAME): CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -I$(CAMLIB_SRC)/ -Iwindows-gtk/win64-gtk-2021/win32/include
win64-gtk-$(APP_NAME): win.res windows-gtk/win64-gtk-2021 $(WIN_FILES) $(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll
	rm -rf win64-gtk-$(APP_NAME)/
	cp $(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll libwpd_x64.dll
	-mkdir win64-gtk-$(APP_NAME)
	$(CC) win.res $(WIN_FILES) windows-gtk/win64-gtk-2021/win32/lib/*.dll libwpd_x64.dll $(CFLAGS) -o win64-gtk-$(APP_NAME)/$(APP_NAME).exe
	cp libwpd_x64.dll win64-gtk-$(APP_NAME)/libwpd.dll
	cp windows-gtk/win64-gtk-2021/win32/lib/*.dll win64-gtk-$(APP_NAME)/
	cp assets/README.txt win64-gtk-$(APP_NAME)/

# 32 bit for all the ReactOS nerds to try
win32-gtk: win32-gtk-$(APP_NAME)
win32-gtk-$(APP_NAME): MINGW=i686-w64-mingw32
win32-gtk-$(APP_NAME): CC=$(MINGW)-gcc
win32-gtk-$(APP_NAME): CFLAGS=-s -lws2_32 -lkernel32 -lurlmon -I$(CAMLIB_SRC)/ -Iwindows-gtk/win32-gtk-2013/win32/include
win32-gtk-$(APP_NAME): win.res windows-gtk/win32-gtk-2013 $(WIN_FILES) libwpd_x86.dll
	-mkdir win32-gtk-$(APP_NAME)
	$(CC) win.res $(WIN_FILES) windows-gtk/win32-gtk-2013/win32/lib/*.dll libwpd_x86.dll $(CFLAGS) -o win32-gtk-$(APP_NAME)/$(APP_NAME).exe
	cp libwpd_x86.dll win32-gtk-$(APP_NAME)/libwpd.dll
	cp windows-gtk/win32-gtk-2013/win32/lib/* win32-gtk-$(APP_NAME)/
	cp assets/README.txt win32-gtk-$(APP_NAME)/
