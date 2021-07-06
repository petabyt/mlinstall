# Quick script to kill stray tkinter processes:
# kill -9 `ps -e | grep python3 | awk '{printf $1}'`

import ptpy
from ptpy import Canon
from tkinter import *
from time import sleep
import json

from canon import *
from install import *

nodownload = 0
try:
	import requests
except:
	print("Can't find requests, won't download.")
	nodownload = 1

# Disable downloading for now, unfinished
nodownload = 1

ws = Tk()
ws.title("Magic Lantern USB Tool")
ws.geometry("450x600+700+200")

output = ""

camera = None

def log(text):
    print(text)

    global output
    output = output + text + "\n"
    output_label.config(text = output)
    ws.update()

def connect():
    global output
    output = ""

    global camera
    log("Attempting to connect to camera...")
    if camera == None:
        try:
           camera = ptpy.PTPy()
        except Exception as e:
            print("Connection Exception: " + str(e))
            if "No USB PTP device found." in str(e):
                log("No camera found. It might be mounted.")
            elif "No backend available" in str(e):
                log("Could not find libusb backend.")
                log("Use Zadig to install libusbk.")
            return 1
    else:
        log("Already connected to camera")
    return 0

def check():
    if connect():
        return 1

    with camera.session():
        info = camera.get_device_info()
        if isCanon(info.Model):
            log("Model: " + info.Model)
            log("Firmware version: " + info.DeviceVersion[2:])
            return 0
        else:
            log("Sorry, " + info.Model + " probably isn't supported.")
            return 1

def bootflag_on():
    if check():
        return 1
    with camera.session():
        result = camera.eos_run_command("EnableBootDisk")
        print(result)
        if result.ResponseCode == "OK":
            log("Boot flag enabled.")
        else:
            log("Could not enable boot flag")

def bootflag_off():
    if check():
        return 1
    with camera.session():
        result = camera.eos_run_command("DisableBootDisk")
        if result.ResponseCode == "OK":
            log("Boot flag disabled.")
        else:
            log("Could not disable boot flag")

def run_custom(Event = None):
    if connect():
        return

    with camera.session():
        r = camera.eos_run_command(custom_entry.get())
        print(r)
        log("Session ID: " + str(r.SessionID))
        log("Transaction ID: " + str(r.TransactionID))
        log("Response Code: " + str(r.ResponseCode))
        log("Response Value: " + str(r.Parameter[0]))

def install_ml():
    if connect():
        return 1

    version = None
    with camera.session():
        version = info.DeviceVersion[2:]

    if version == None:
        log("Could not get version")
        return

    info = getML("EOS Canon Rebel T6", version)
    if not info:
        log("Failed to download Magic Lantern for\n" + name + " firmware version " + version)

    log("Downloading ML from " + info["ml"])

    f = open("ml.zip", "wb")
    r = requests.get(info["ml"], allow_redirects=True, stream=f)
    log("Done. Unzip ml.zip onto your SD card or mounted camera.")

# Main UI Setup

welcome_label = Label(
    ws,
    text = "Magic Lantern USB Installer",
    font = ("Arial", 20)
).pack(fill = BOTH, expand = False)

welcome_label = Label(
    ws,
    text = "THIS IS NOT GARUNTEED TO WORK\nOR NOT KILL YOUR CAMERA\nKEEP BOTH PIECES IF YOU BREAK IT",
    font = ("Arial", 10)
).pack(fill = BOTH, expand = False)

Button(
    ws,
    text = "Connect",
    command = check,
    bg = "#d1d1d1"
).pack(fill = BOTH, expand = False)

bootflag_on_button = Button(
    ws,
    text = "Enable boot flag",
    command = bootflag_on,
    bg = "#d1d1d1"
).pack(fill = BOTH, expand = False)

Button(
    ws,
    text = "Disable boot flag",
    command = bootflag_off,
    bg = "#d1d1d1"
).pack(fill = BOTH, expand = False)

if not nodownload:
    install_ml_button = Button(
        ws,
        text = "Install latest Magic Lantern",
        command = install_ml,
        bg = "#d1d1d1"
    ).pack(fill = BOTH, expand = False)

output_label = Label(
    ws,
    text = "Output goes here...",
    font = ("Arial", 13)
)

custom_label = Label(
    ws,
    text = "Run Custom DryOS Shell Command:",
    font = ("Arial", 13)
)

custom_entry = Entry(
    ws,
    bg = "#d1d1d1"
)

custom_entry.bind("<Return>", run_custom)
custom_entry.insert(0, "TurnOffDisplay")


install_ml_button = Button(
    ws,
    text = "Make EOS_DIGITAL Card Bootable",
    command = install,
    bg = "#d1d1d1"
).pack(fill = BOTH, expand = False)


custom_label.pack(fill = BOTH, expand = False)
custom_entry.pack(fill = BOTH, expand = False)

output_label.pack(fill = BOTH, expand = False)

ws.mainloop()