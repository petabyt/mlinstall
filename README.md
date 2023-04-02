# mlinstall
A Windows/Linux app to assist with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

![screenshot](https://eggnog.theres.life/f/27-4i8br89x4xfu20tdqrylznyu6stl09.png)

## Features
- Enable/Disable boot disk (hopefully replacing custom encrypted firmware files)
- Basic PTP functionality (get firmware/model/serial info)
- Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- Make card Canon Basic Scriptable
- Destroy card flags (without reformat)
- App Store: Downloads and installs modules on SD card

## TODO / HELP NEEDED:
 - Fuzz testing
 - Mac OS port (don't have a mac)

All pull requests/issues are welcome.  

## Linux Compilation
Clone with `--recurse-submodules`.
```
# Install required packages:
libusb-dev gcc libgtk-3-dev

# Compile Linux GTK app
make unix-gtk
```

For releases, staticx (`pip3 install staticx`) is used to  
convert dynamic executables to static. (similar to AppImage)  

## MacOS Compilation
camlib works on MacOS, but mlinstall/GTK hasn't been ported yet.  
The Python3-based version should work, see [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  

## Windows Compilation
Cross compile for Windows, from Linux some libs must be downloaded to provide  
the DLLs for the compiler and zip file. MinGW is required (`apt install gcc-mingw-w64-x86-64`)  

```
make win64-gtk-mlinstall.zip
```

Licensed under GNU General Public License v2.0.  
