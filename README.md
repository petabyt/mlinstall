# mlinstall
A Windows/Linux app to help with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

The backend is a fork of [ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  

A Python based version of this app is available at [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  
You can find standalone prebuilts for it [here](https://github.com/petabyt/mlinstall/releases/tag/0.1.0).

![screenshot](screenshot.png)

## Features
- [x] - Enable/Disable boot disk
- [x] - Execute DryOS shell commands (event procedures)
- [x] - Basic PTP functionality
- [x] - Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- [x] - Destroy card flags

## TODO:
- [ ] - Lots of fuzz testing
- [ ] - Download correct ML based on model and FW version (?)
- [ ] - Clean up makefiles
- [ ] - Get WiFi ptp working
- [ ] - Try on Mac

## Compilation
Compile for Linux:  
```
# requires gcc/tcc, libusb-dev
# (sudo apt install libusb-dev)
make

# Compile GTK-based program
make gui
```

Compile from Windows:  
```
rem Requires libusb + x86_64-w64-mingw32-gcc.
rem See make.bat
make.bat
```

Cross compile for Windows, from Linux  
Some libs must be downloaded to provide  
the DLLs for the compiler and zip file.  
Requires `x86_64-w64-mingw32`.  

```
# Compile and pack gui program zip
make setuplibs
make windowsgtk
make windowsgtkpack
make removelibs

# Compile and pack cli program zip
make setuplibs
make windows
make windowspack
make removelibs
```


Licensed under `GNU General Public License v2.0`.
`ptpcam  (c)2001-2006 Mariusz Woloszyn <emsi@ipartners.pl>`  
Magic Lantern fork: Minor changes by g3gg0 and nanomad  
This fork: Applied some research from https://github.com/petabyt/sequoia-ptpy  
(also formatted to kernel style, and removed cli part)
