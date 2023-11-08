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
 - [ ] Display FW and internal build version
 - [ ] Automatically connect to camera
 - [ ] Automatically detect card
 - [ ] Upload arbritrary files to the card (.bin, .mo, etc)
 - [ ] MacOS port (help needed)
 - [ ] Diagnostic + error reporting

All pull requests/issues are welcome.

## Linux Compilation
Clone with `--recurse-submodules`.
```
# Install required packages:
sudo apt install libusb-dev gcc libgtk-3-dev
# Compile Linux GTK app
make unix-gtk && ./unix-gtk
```

For releases, staticx (`pip3 install staticx`) is used to  
convert dynamic executables to static. (functionally the same as AppImage)  

## Windows Compilation
Mingw is required: `apt install gcc-mingw-w64-x86-64`. Other than that, the windows-gtk submodule has everything else needed to compile:
```
make win64-gtk-mlinstall.zip
```

Licensed under GNU General Public License v2.0.  
