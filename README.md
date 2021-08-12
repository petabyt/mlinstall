# mlinstall
A Windows/Linux app to help with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

The backend is a [modified](https://github.com/petabyt/sequoia-ptpy) fork of [ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  
A Python based version of this app is available at [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  

![screenshot](assets/screenshot.png)

## Features
- Execute any DryOS event procedure
- Enable/Disable boot disk
- Basic PTP functionality (get camera info)
- Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- Make card Canon Basic Scriptable
- Destroy card flags (without reformat)

## TODO / Help Needed:
- [ ] - Rename app. MLTools? (it may do more than just installation-related things)
- [ ] - General bug testing, make user friendly
- [ ] - Fuzz testing
- [ ] - Download correct ML based on model and FW version [(?)](https://developers.canon-europe.com/developers/s/article/Latest-EOS-SDK-Version-3-x)  
Would have to make sure [model_get](https://github.com/petabyt/mlinstall/blob/master/src/model.c#L43) works correctly,  
and compile a [list](https://github.com/petabyt/mlinstall/blob/python-stable/canon.py#L52) of Magic Lantern releases.  
- [ ] - Get WiFi ptp working [(?)](https://github.com/Parrot-Developers/sequoia-ptpy/issues/18)  
Might be useful if the USB port is [damaged](https://www.cloudynights.com/topic/497224-any-experiences-on-repairing-usb-port-on-canon-dslr/).  
- [ ] - Try on Mac (don't own a modern one)
- [ ] - Improve the [manual](MANUAL.md)
- [ ] - Possibly replace it with a [Magic Lantern Wiki](https://wiki.magiclantern.fm/start) page.
- [ ] - Allow integer parameters in "Custom Event Procedure" option.  
Feel free to make pull request or issue.  

## Unix Compilation
```
# requires C99 compiler and libusb-dev
# (sudo apt install libusb-dev)
make unix-gtk

# Compile CLI-based program
make unix-cli
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
This fork: Applied some research from https://github.com/petabyt/sequoia-ptpy  
into the [Magic Lantern fork of ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  