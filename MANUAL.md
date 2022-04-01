# MLInstall Manual

Trying to install ML for first time? See [First time install](#HOW-TO-INSTALL-MAGIC-LANTERN-WITH-MLINSTALL).

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

## First time Magic Lantern install with MLINSTALL
Written by Daniel C and Sebastián Jaiovi

Welcome to this fast-tutorial. Get sure:
1. You "upgraded" the camera firmware to compatible version with the nightly build
2. Formatted the card, preferably in the camera
3. Be conscious of ML philosophy: if you break it, you keep the two pieces.

mlinstall will help you install any nightly build zip that doesn't have the boot file. *Example: I'm a Canon T6 user* 

### Steps
1: Connect the camera to a Windows 7/10 or Linux computer with a USB cable. 
2: Download mlinstall (https://github.com/petabyt/mlinstall). Click on “Get Device Info” to see if it can detect the device.
![image4](https://user-images.githubusercontent.com/72230570/161169243-195e2c77-5785-4af2-ab0f-376cb2486093.png)


For windows users, it won't work natitavely. You need to:
- Download and run Zadig.
- With your camera driver selected, change "WinUSB" with the tiny arrows to "libusb-win32"
- Install WCID Driver and re-plug it. Restart mlinstall.
- In order to revert this change, simply repeat the steps and choose “WinUSB” instead of “libusb-win32”. *Some Windows software may fail to work if this change is not reverted.*

Then:
3. Once you have verified that USB communication is working, click “Enable Boot Disk”. *In order to revert the camera to factory settings, click “Disable Boot Disk”.*
![image2](https://user-images.githubusercontent.com/72230570/161169536-f24ce6bc-52ab-447b-875c-a5300238a56e.png)

5. Unplug the SD card from the camera.
6. Insert the SD card into your computer.
7. Ensure that your SD card is named “EOS_DIGITAL”. mlinstall searches for any storage device named “EOS_DIGITAL” and writes to the first one it finds.
![image5](https://user-images.githubusercontent.com/72230570/161169636-22559b01-e6a4-4a02-9c1f-5f3a709a03a5.png)

9. In mlinstall, navigate to the “Card” tab and click “Write card boot flags”. This will write the data to the SD card. *In Linux, eject the card before removing it. mlinstall currently doesn’t support exfat, so you might want to use EOSCard.*

8. Insert the SD card into your camera and turn it on.

Few tips:
- ML could be intimedating at first. Your first and most important menu in ML is the trashcan button when you're in live view. 
- In submenu Modules (the four squares) you can activates the modules you want.
- Some modules are messy and could overheat your camera. Be careful with recording RAW or too much active modules at the same time.


## FAQ

### No PTP/USB device found
You will most likely have to replace WinUSB with libusb if  
you are running Windows. In order to do this, download [Zadig](https://zadig.akeo.ie/)  
and replace WinUSB with libusb-win32.  

![animation](assets/zadig.gif)
