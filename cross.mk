# Cross-compilation stubs for GNU make
_all: all

# Autoselect ARCH based on one letter TARGET
# ARCH will also be autoselected from TARGET if farther down
ifndef TARGET
  ifneq ($(findstring linux,$(ARCH)),)
    TARGET := l
  endif
  ifneq ($(findstring mingw,$(ARCH)),)
    TARGET := w
  endif
  ifndef TARGET
    $(warning TARGET not defined, assuming Linux)
    TARGET := l
  endif
endif

ifeq ($(TARGET),w)
ARCH ?= x86_64-w64-mingw32
LIB_DIR := /usr/$(ARCH)-w64-mingw32/lib
INC_DIR := /usr/$(ARCH)-w64-mingw32/include

LIBWPD_A := /usr/$(ARCH)/lib/libwpd.a
LIBLUA_A := /usr/$(ARCH)/lib/liblua.a
LIBUI_DLL := /usr/$(ARCH)/lib/libui_win64.dll
LIBUI_A := /usr/$(ARCH)/lib/libui.a

WIN_LINK_ESSENTIALS += -luser32 -lkernel32 -lgdi32 -lcomctl32 -luxtheme -lmsimg32 -lcomdlg32 -ld2d1 -ldwrite -lole32 -loleaut32 -loleacc -lssp -lurlmon -luuid -lws2_32

%.res: %.rc
	$(ARCH)-windres $< -O coff -o $@
endif

ifeq ($(TARGET),l)
ARCH ?= x86_64-linux-gnu
LIB_DIR := /usr/lib
INC_DIR := /usr/include

# Create appimages
# TODO: Link to linuxdeploy and appimagetool
define create_appimage
linuxdeploy --appdir=AppDir --executable=$1.out -d assets/$1.desktop -i assets/$1.png
appimagetool AppDir
endef

linuxdeploy:
	echo "Download from https://github.com/linuxdeploy/linuxdeploy/releases/"

endif

CC := $(ARCH)-gcc
CPP := $(ARCH)-c++
CXX := $(ARCH)-c++
GPP := $(ARCH)-g++
PKG_CONFIG := $(ARCH)-pkg-config
AR := $(ARCH)-ar

# Convert object list to $(TARGET).o
# eg: x.o a.o -> x.w.o a.w.o if $(TARGET) is w
define convert_target
$(patsubst %.o,%.$(ARCH).o,$1)
endef

%.$(ARCH).o: %.c
	$(CC) -MMD -c $< $(CFLAGS) -o $@

%.$(ARCH).o: %.cpp
	$(CXX) -MMD -c $< $(CFLAGS) -o $@

%.$(ARCH).o: %.cxx
	$(CXX) -MMD -c $< $(CXXFLAGS) $(CFLAGS) -o $@

%.$(ARCH).o: %.S
	$(CC) -c $< $(CFLAGS) -o $@
