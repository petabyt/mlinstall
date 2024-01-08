WIN_FILES:=$(APP_CORE) src/libui.o src/drive-win.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libwpd.o)
WIN_FILES:=$(call convert_target,$(WIN_FILES))

MINGW=x86_64-w64-mingw32
CC=$(MINGW)-gcc
CPP=$(MINGW)-c++

# External dependencies
LIBWPD_A?=../libwpd/libwpd_64.a
LIBUI_A?=/usr/x86_64-w64-mingw32/lib/libui.a

LIBWPD_VER=0.1.4

$(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/$(LIBWPD_VER)/libwpd_64.dll -O $(DOWNLOADS)/libwpd_x64-$(LIBWPD_VER).dll

$(DOWNLOADS)/libwpd_x86-$(LIBWPD_VER).dll:
	-wget -4 -nc https://github.com/petabyt/libwpd/releases/download/$(LIBWPD_VER)/libwpd_32.dll -O $(DOWNLOADS)/libwpd_x86-$(LIBWPD_VER).dll

$(DOWNLOADS)/libui_win64.a:
	-wget -4 -nc https://github.com/petabyt/libui-cross/raw/master/libui.a -O $(DOWNLOADS)/libui_win64.a

# Contains app info, asset stuff
win.res: assets/win.rc
	$(MINGW)-windres assets/win.rc -O coff -o win.res

LIBS=-luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc
LIBS+=-lstdc++ -lgcc -static -s -lpthread -lssp
LIBS+=-lurlmon

# Remove cmd window from startup
LIBS+=-Wl,-subsystem,windows

windows: mlinstall.exe

mlinstall.exe: $(WIN_FILES) win.res win.mak $(DOWNLOADS)/libui_win64.a
	$(CC) win.res $(WIN_FILES) $(LIBUI_A) $(LIBWPD_A) $(LIBS) -o $(APP_NAME).exe

mlinstall_x86_64.exe: mlinstall.exe
	cp mlinstall.exe mlinstall_x86_64.exe

copy:
	cp mlinstall.exe /mnt/c/Users/brikb/OneDrive/Desktop/
