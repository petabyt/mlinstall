# For Linux only

all:
	@python3 main.py

setup:
	@sudo apt install python3-tk pip3 python3
	@pip3 install tk
	@pip3 install "sequoia-ptpy/"

pack:
	@pyinstaller -D -y --onefile main.py
	mv dist/main dist/mlinstall_x86_64_linux

clean:
	@rm -rf __pycache__ build dist