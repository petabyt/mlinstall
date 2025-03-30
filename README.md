# MLinstall
A Windows/Linux/Mac app to assist with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

![screenshot](assets/screenshot.png)

## Features
- Make camera bootable over USB (alternative to Magic Lantern's custom .FW files)
- Get firmware & build version, get shutter actuation count, other info
- Write memory card boot and script flags (FAT16/32/ExFAT)
- Clear card flags without reformattting
- Shutter counter
- Shows internal FW build version

## Compiling
- Clone with `--recurse-submodules`.
- Required packages: `sudo apt install libusb-dev gcc libgtk-3-dev`
```
cmake -G Ninja -B build && cmake --build build
```
Licensed under GNU General Public License v2.0.  
