@echo off
rem (This is probably unmaintained)

rem Windows Compilation script
rem Usage:
rem make.bat           Compile default CLI app
rem make.bat gtk       Compile gtk app

rem Windows users:
rem Download and unzip [libusb-win32](https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip/download)
rem Install `x86_64-w64-mingw32-gcc`

rem This script might work:
rem curl.exe https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip > libusb.zip
rem unzip libusb

rem Should be in top directory:
set LIBUSB=libusb-win32-bin-1.2.2.0

rem !! Required compiler (compatibility)
set CC=x86_64-w64-mingw32-gcc
set FILES=drive-win.c drive.c ptpcam.c myusb.c properties.c ptp.c

set CFLAGS=-I%LIBUSB%/include -I../%LIBUSB%/include
set LINK=-lws2_32 -lkernel32 "../%LIBUSB%/bin/amd64/libusb0.dll"

where %CC% >nul
if %ERRORLEVEL% neq 0 (
	echo !!!! %CC% Not found. Download it using Cygwin.
	exit /b
)

if NOT EXIST %LIBUSB% (
	echo !!!! Libusb not found. Download it from:
	echo !!!! https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	exit /b
)

rem TODO: Won't compile gtk.c
rem Compile gtk.c
rem mingw-gtk3.0
rem mingw-glib2.0
if "%1" == "gtk" (
	cd src
	%CC% %CFLAGS% ../gtk.c %FILES% %LINK% -o ../ptpcam.exe
	cd ..
	exit /b
)

rem Default console app
cd src
%CC% %CFLAGS% ../main.c %FILES% %LINK% -o ../ptpcam.exe
cd ..
