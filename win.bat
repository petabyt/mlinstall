@echo off
copy winspec main.spec
pyinstaller -w --onefile -D main.spec
del main.spec