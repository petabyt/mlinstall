# mlinstall
A Windows/Linux app to help with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

The backend is a fork of [ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  

A Python based version of this app is available at [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  
![screenshot](screenshot.png)

## Features
- [x] - Enable/Disable boot disk
- [x] - Execute DryOS shell commands (event procedures)
- [x] - Basic PTP functionality
- [x] - Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- [x] - Destroy card flags

## TODO:
- [ ] - LOTS of fuzz testing
- [ ] - Download correct ML based on model and FW version [?](https://developers.canon-europe.com/developers/s/article/Latest-EOS-SDK-Version-3-x)
- [ ] - Clean up makefiles
- [ ] - Get WiFi ptp working
- [ ] - Try on Mac

## Unix Compilation
```
# requires C99 compiler, libusb-dev
# (sudo apt install libusb-dev)
make unix-cli

# Compile GTK-based program
make unix-gtk
```

Cross compile for Windows, from Linux  
Some libs must be downloaded to provide  
the DLLs for the compiler and zip file.  
You must use `gcc-mingw-w64-x86-64`.  

```
# Compile and pack gui program zip
make win-libs
make win-gtk
make win-gtk-pack

# Compile and pack cli program zip
make win-libs
make win-cli
make win-cli-pack
```

Licensed under `GNU General Public License v2.0`.  
`ptpcam  (c)2001-2006 Mariusz Woloszyn <emsi@ipartners.pl>`  
Magic Lantern fork: Minor changes by g3gg0 and nanomad  
This fork: Applied some research from https://github.com/petabyt/sequoia-ptpy  
(also formatted to kernel style, and removed cli part)  