@echo off

rem Windows users:
rem Download and unzip [libusb-win32](https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip/download)
rem Install `x86_64-w64-mingw32-gcc`

rem This script might work:
rem curl.exe https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip > libusb.zip
rem unzip libusb

rem !! Required compiler (compatibility)
set CC=x86_64-w64-mingw32-gcc
set CFLAGS=-I"libusb-win32-bin-1.2.2.0/include"
set LINK=-lws2_32 -lkernel32 "libusb-win32-bin-1.2.2.0/bin/amd64/libusb0.dll"

where %CC% >nul
if %ERRORLEVEL% neq 0 (
	echo !!!! %CC% Not found. Download it using Cygwin.
	exit /b
)

%CC% %CFLAGS% main.c flag.c ptpcam.c myusb.c properties.c ptp.c %LINK% -o ptpcam.exe
