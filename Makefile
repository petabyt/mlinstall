# For Linux only

# Pack into single executable
F := -D -y --onefile -F

default:
	@python3 main.py

help:
	@echo "make         Test main.py"
	@echo "make setup   Setup libs"
	@echo "make pack    Run pyinstaller"
	@echo "make clean"

setup:
	-@sudo apt install python3-tk python3-pip python3
	@pip3 install tk
	@pip3 install pyusb
	@pip3 install pyinstaller
	@pipe install staticx
	@cd seq*; pip3 install .

pack:
	@cp mainspec main.spec
	@python3 -m PyInstaller $(F) main.spec

# Pack with glibc into single supermassive 20+ megabyte static executable
	@staticx dist/main dist/mlinstall_x86_64_linux

clean:
	@rm -rf __pycache__ build dist *.spec