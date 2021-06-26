all:
	@python3 main.py

setup:
	@sudo apt install python3-tk pip3 python3
	@pip3 install tk
	@pip3 install "sequoia-ptpy/"

pack:
	@pyinstaller main.py