import ptpy
from ptpy import Canon

from tkinter import *

from time import sleep
from urllib.request import urlopen
import base64

ws = Tk()
ws.title("Magic Lantern USB Installer")
ws.geometry("450x450+700+200")

output=""

camera = None

def log(text):
    print(text)
    
    global output
    output = output + text + "\n"
    output_label.config(text=output)
    ws.update()

def connect():
    global output
    output=""
    
    global camera
    log("Attempting to connect to camera...")
    if camera == None:
        try:
            camera = ptpy.PTPy()
            log("Connected to the camera.")
        except:
            log("No camera found! Make sure it isn't mounted.")
            return None
    else:
        log("Already connected to camera")
    return 1

def install():
    if connect() == None:
        return

    with camera.session():
        info = camera.get_device_info()
        log("Model: " + info.Model)
        log("Firmware version: " + info.DeviceVersion[2:])
        
        result = camera.eos_run_command("EnableBootDisk")
        if result.ResponseCode == 'OK':
            log("Boot flag enabled.")
        else:
            log("Could not enable boot flag")

def run_custom(Event=None):
    if connect() == None:
        return

    with camera.session():
        r = camera.eos_run_command(custom_entry.get())
        print(r)
        log("Session ID: " + str(r.SessionID))
        log("Transaction ID: " + str(r.TransactionID))
        log("Response Code: " + str(r.ResponseCode))
        log("Response Value: " + str(r.Parameter[0]))

bootflag_button = Button(
    ws,
    text="Enable boot flag",
    command=install,
    bg='#d1d1d1'
)

output_label = Label(
    ws,
    text="Output goes here...",
    font=('Arial', 13)
)

custom_label = Label(
    ws,
    text="Run Custom DryOS Shell Command:",
    font=('Arial', 13)
)

custom_entry = Entry(
    ws,
    bg='#d1d1d1'
)

custom_entry.bind('<Return>',run_custom)

custom_entry.insert(0, "TurnOffDisplay")

bootflag_button.pack(fill=BOTH, expand=False)
custom_label.pack(fill=BOTH, expand=False)
custom_entry.pack(fill=BOTH, expand=False)
output_label.pack(fill=BOTH, expand=False)

ws.mainloop()
