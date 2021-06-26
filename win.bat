@echo off
copy win.spec main.spec
pyinstaller -w --onefile -D main.spec
del main.spec