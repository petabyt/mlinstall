# mlinstall
A Windows/Linux app to help with installing Magic Lantern.  
Download: https://github.com/petabyt/mlinstall/releases  

![screenshot](assets/screenshot.png)

## Features
- Execute any DryOS event procedure with parameters
- Enable/Disable boot disk (possibly replacing custom encrypted firmware files)
- Basic PTP functionality (get firmware/model/serial info)
- Write EOS_DEVELOP and BOOTFLAG to card (Windows + Linux)
- Make card Canon Basic Scriptable
- Destroy card flags (without reformat)

## TODO / Help Needed:
 - Rename app. Magic Lantern App? (it may do more than just installation-related things)
 - General bug testing, make user friendly
 - Fuzz testing
 - Get WiFi ptp working [(?)](https://github.com/Parrot-Developers/sequoia-ptpy/issues/18)
Might be useful if the USB port is [damaged](https://www.cloudynights.com/topic/497224-any-experiences-on-repairing-usb-port-on-canon-dslr/).  
Update: See https://diode.zone/w/cGnnBDjSmjFiYFVVn7rzR2
 - Try on Mac (I have an [iMac G4](https://en.wikipedia.org/wiki/IMac_G4), is that too old?)
 - Improve the [user manual](MANUAL.md)
 - Possibly replace it with a [Magic Lantern Wiki](https://wiki.magiclantern.fm/start) page.
 - Avoid using system shell commands
Feel free to make pull request or issue.  

## Linux Compilation
Required packages:  
- libusb-dev
- gcc or tcc
- libgtk-3-dev
```
make unix-gtk
```

For releases, staticx (`pip3 install staticx`) is used to  
convert dynamic executables to static. (similar to AppImage)  

## MacOS Compilation
Not ported to MacOS yet. The Python3-based version should work, see [python-stable](https://github.com/petabyt/mlinstall/tree/python-stable).  

## Windows Compilation
Cross compile for Windows, from Linux  
Some libs must be downloaded to provide  
the DLLs for the compiler and zip file.  
MinGW is required (`apt install gcc-mingw-w64-x86-64`)  

```
# Compile and pack gui zip
make win-libs
make win-gtk
make win-gtk-pack
```

Licensed under `GNU General Public License v2.0`.  
`ptpcam  (c)2001-2006 Mariusz Woloszyn <emsi@ipartners.pl>`  
This fork: Applied some research from https://github.com/petabyt/sequoia-ptpy  
into the [Magic Lantern fork of ptpcam](https://github.com/reticulatedpines/magiclantern_simplified/tree/dev/contrib/ptpcam).  
