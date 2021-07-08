# mlinstall
A cross platform app to help with installing Magic Lantern.  

The current app is based on [ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  

A Python based version of this app is available at [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  
You can find standalone prebuilts here: https://github.com/petabyt/mlinstall/releases/tag/0.1.0

## Features
- [x] - Enable/Disable boot disk
- [x] - Execute DryOS shell commands
- [x] - Basic PTP functionality
- [x] - Write EOS_DEVELOP and BOOTFLAG to SD card (Windows + Linux)
- [ ] - Download correct ML based on model and FW version (?)

## Compilation
```
rem Requires libusb + x86_64-w64-mingw32-gcc.
rem See make.bat
make.bat
```

```
# requires gcc/tcc, libusb-dev
make
```
