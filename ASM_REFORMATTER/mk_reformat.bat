@echo off
call vcvars32.bat
cl reformat.c
if errorlevel 1 pause && exit /B %ERRORLEVEL%
del reformat.obj
reformat -i:cpm22_Z80MBC2.asm -o:_cpm22_Z80MBC2.asm -u
for %%f in ( bdos bios boot ccp m4sysgen ) do reformat -i:%%f.asm -o:_%%f.asm -u -m:2 -c:5 
for %%f in ( os2ccp os3bdos ) do reformat -i:%%f.asm -o:_%%f.asm -u -m:1 -c:4 -s:! -xs -xn -xz
for %%f in ( os1boot os4bios ) do reformat -i:%%f.asm -o:_%%f.asm -u -xz
