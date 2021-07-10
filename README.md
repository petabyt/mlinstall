# mlinstall
A Windows/Linux app to help with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

The backend is a fork of [ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  

A Python based version of this app is available at [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  
You can find standalone prebuilts for it [here](https://github.com/petabyt/mlinstall/releases/tag/0.1.0).

**This is not guaranteed to work perfectly  
or not brick your camera. Use at your own risk.**  

## Features
- [x] - Enable/Disable boot disk
- [x] - Execute DryOS shell commands (event procedures)
- [x] - Basic PTP functionality
- [x] - Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- [x] - Destroy card flags
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

![screenshot](screenshot.png)
