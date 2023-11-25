cp /opt/local/lib/libusb-1.0.0.dylib macos/application/
cp /usr/local/lib/libui.dylib macos/application/
cp linux macos/application/mlinstall.app
bash macos/build-macos-x64.sh mlinstall 1.1.0
