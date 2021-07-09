@echo off
rem Usage:
rem make.bat           Compile default CLI app
rem make.bat gtk       Compile gtk app

rem Windows users:
rem Download and unzip [libusb-win32](https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip/download)
rem Install `x86_64-w64-mingw32-gcc`

rem This script might work:
rem curl.exe https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip > libusb.zip
rem unzip libusb

rem !! Required compiler (compatibility)
set CC=x86_64-w64-mingw32-gcc
set CFLAGS=-I"%LIBUSB%/include"

rem (compiled while in src)
set LIBUSB=../libusb-win32-bin-1.2.2.0
set LINK=-lws2_32 -lkernel32 "%LIBUSB%/bin/amd64/libusb0.dll"

set FILES=flag.c ptpcam.c myusb.c properties.c ptp.c

where %CC% >nul
if %ERRORLEVEL% neq 0 (
	echo !!!! %CC% Not found. Download it using Cygwin.
	exit /b
)

dir %LIBUSB% >nul
if %ERRORLEVEL% neq 0 (
	echo !!!! Libusb not found. Download it from:
	echo !!!! https://cfhcable.dl.sourceforge.net/project/libusb-win32/libusb-win32-releases/1.2.2.0/libusb-win32-bin-1.2.2.0.zip
	exit /b
)

rem Compile gtk.c
if %1 == "gtk" (
	cd src
	%CC% %CFLAGS% ../gtk.c %FILES% %LINK% -o ../ptpcam.exe
	cd ..
	exit /b
)

rem Default, CLI app
cd src
%CC% %CFLAGS% ../main.c %FILES% %LINK% -o ../ptpcam.exe
cd ..
