# mlinstall
A Windows/Linux app to assist with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

![screenshot](assets/screenshot.png)

## Features
- Make camera bootable over USB (replacing custom signed firmware files)
- Get firmware & build version, get shutter actuation count, other info
- Write memory card boot and script flags - all existing functionality of EOSCard
- Clear card flags without reformattting
- ~~App Store: Downloads and installs modules on SD card~~ removed for now

## Roadmap
 - [x] Add shutter counter
 - [x] Display FW and internal build version
 - [ ] Automatically connect to camera
 - [ ] Automatically detect card
 - [ ] Upload arbritrary files to the card (.bin, .mo, etc)
 - [x] MacOS port
 - [ ] Diagnostic + error reporting

All pull requests/issues are welcome.

## Linux Compilation
- Clone with `--recurse-submodules`.
- libui-cross is required - you can install it through https://github.com/petabyt/libui-cross
- Required packages: `sudo apt install libusb-dev gcc libgtk-3-dev`
```
make linux
```

## Windows Compilation
- Mingw is required: `apt install gcc-mingw-w64-x86-64`.
- libui-cross can be compiled and installed for cross-compilation on WSL/Linux
```
make TARGET=w mlinstall.exe
```

## MacOS Compilation
- Can be compiled in [darling](https://darlinghq.org) for x86_64
- libs: https://s1.danielc.dev/filedump/libs.tar.gz
- TODO: more info
- Does it work on M1?

Licensed under GNU General Public License v2.0.  
