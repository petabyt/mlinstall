#!/bin/python3
# Standalone Python script for enabling boot flag on Canon cameras
# Requires ptpy

import ptpy
from construct import Container
import sys

if sys.argc != 2:
	print("Usage: python3 boot.py <command>")
	print("So to enable boot disk: python3 boot.py EnableBootDisk")

string = sys.argv[1]

# Custom lib hijacks:
def custom_send(self, code, data, param = []):
    ptp = Container(
        OperationCode=code,
        SessionID=self._session,
        TransactionID=self._transaction,
        Parameter=param
    )

    response = self.send(ptp, data)
    return response

setattr(ptpy.PTP, "custom_send", custom_send)

def custom_mesg(self, code, param = []):
    ptp = Container(
        OperationCode=code,
        SessionID=self._session,
        TransactionID=self._transaction,
        Parameter=param
    )

    response = self.mesg(ptp)
    return response

setattr(ptpy.PTP, "custom_mesg", custom_mesg)

camera = ptpy.PTPy()

with camera.session():
	# Enable 9052 on newer cameras (EOSEnableCommand)
	print(camera.custom_send(0x9050))
	print(camera.custom_send(0x9050))
	print(camera.custom_send(0x9050))

	data = string.encode()

	# This is for the evproc parameters, but leaving it blank will do
	data += bytesarray(100)

    print(camera.custom_send(0x9052, data))

