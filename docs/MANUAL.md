# MLInstall Manual
...

## USB
This will commands on your camera via PTP/USB.

### Get device info
Prints firmware version, model, serial number, etc.

### Enable boot disk
Allows the camera to run custom code from SD/CF card, if BOOTDISK  
is written into the SD card. See [Write card boot flags](#user-content-write-card-boot-flags).  

### Disable boot disk
Disables the boot disk, so that the camera won't look for  
autoexec.bin. You might also want to [Destroy card boot flags](#user-content-destroy-card-boot-flags).  

### Custom Event Procedures
Mlinstall has a built-in event procedure command parser. Event procedures (also known as EvProcs)  
are special commands that can be sent to the camera via USB. These commands can be used to change  
certain settings or perform certain tests.  
Mlinstall parses strings (between double quotes) and numbers (base 10 and hex). Here's a few quick  
examples:
```
FooBar "Hello, World"
foo_bar 123 0x123
```

Generally the ML developers will provide a firmware file to do this.  

## SD/CF Card
This will write some bytes into the SD/CF card that tells the  
camera to load and execute an autoexec.bin file.  

The utility will write to the first SD/CF card it finds  
named "EOS_DIGITAL". This is the default Canon card name.  

See the [Magic Lantern wiki page](https://wiki.magiclantern.fm/install#installing_magic_lantern_on_other_cards) for more info.

**!! Warning !!**  
**This code will write to your SD card! It's a good idea**  
**to back up all data and format the card in camera first.**  

### Write card boot flags
This will write both EOS_DEVELOP and BOOTDISK.

### Destroy card boot flags
This will write an underscore to the first character of the  
EOS_DEVELOP and BOOTDISK flags. This is useful when you want  
to uninstall Magic Lantern without reformatting the card.  

### Make card scriptable
Writes the SCRIPT flag, allowing the camera to run [Canon BASIC](https://wiki.magiclantern.fm/glossary#canon_basic_scripting).

### Make card un-scriptable
Same procedure as Destroy card boot flags.  

## FAQ

### No PTP/USB device found
You will most likely have to replace WinUSB with libusb if  
you are running Windows. In order to do this, download [Zadig](https://zadig.akeo.ie/)  
and replace WinUSB with libusb-win32.  

![6 Megabyte GIF](https://raw.githubusercontent.com/petabyt/mlinstall/master/assets/zadig.gif)

In order to revert this change, simply repeat the steps and choose “WinUSB” instead of “libusb-win32”. Some Windows software may fail to work if this change is not reverted. 
