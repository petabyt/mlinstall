_WIN_FILES=$(APP_CORE) src/libui.o src/drive-win.o $(addprefix $(CAMLIB_SRC)/,$(CAMLIB_CORE) libwpd.o)
WIN_FILES=$(addsuffix .win.o,$(_WIN_FILES))

MINGW=x86_64-w64-mingw32
CC=$(MINGW)-gcc
CPP=$(MINGW)-c++
CFLAGS+=-I/home/daniel/Pulled/libui-ng
CFLAGS+=-I$(CAMLIB_SRC)/

LIBUI_A=/home/daniel/Pulled/libuiex/libui.a

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
	$(MINGW)-windres -v assets/win.rc -O coff -o win.res

LIBS=-luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc
LIBS+=-lstdc++ -lgcc -static -s -lpthread -lssp
LIBS+=-lurlmon

# Remove cmd window from startup
LIBS+=-mwindows

win-gtk: $(APP_NAME).exe

LIBWPD_A=/home/daniel/Documents/libwpd/libwpd_64.a

$(APP_NAME).exe: $(WIN_FILES) win.res win.mak
	$(CC) win.res $(WIN_FILES) $(LIBUI_A) $(LIBWPD_A) $(LIBS) -o $(APP_NAME).exe

# Release targets:
win64-gtk-$(APP_NAME).zip: win64-gtk-$(APP_NAME)
	zip -r win64-gtk-$(APP_NAME).zip win64-gtk-$(APP_NAME)

%.o.win.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

%.o.win.o: %.S
	$(CC) -c $< $(CFLAGS) -o $@
