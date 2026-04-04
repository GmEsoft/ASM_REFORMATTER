@echo off
cd %~dp0

pushd TEST_CONV

echo - Testing 8080 full instruction set
..\reformat -i:test8080.asm -o:test8080_Z80.asm -xz -xn
..\reformat -i:test8080_Z80.asm -o:test8080_8080.asm -x8 -xn
fc test8080.asm test8080_8080.asm
if errorlevel 1 echo test8080.asm FAILED && exit /b %ERRORLEVEL%

echo - Testing Z-80 full instruction set
..\reformat -i:testz80.asm -o:testz80_8080.asm -x8 -xn -xl
..\reformat -i:testz80_8080.asm -o:testz80_z80.asm -xz -xn -xl
fc testz80.asm testz80_z80.asm
if errorlevel 1 echo testz80.asm FAILED && exit /b %ERRORLEVEL%

echo - Assembling Z-80 version using M80 from Microsoft
..\CPM\simcpm -MA=..\CPM -MB=. -x m80 b:testz80,b:testz80=b:testz80.asm/z
echo Linking Z-80 version using LINK from DRI
..\CPM\simcpm -MA=..\CPM -MB=. -x link b:testz80=b:testz80

set NAME=TESTZ80M
echo 	INCLUDE	Z80.LIB>%NAME%.asm
copy /A %NAME%.asm+testz80_8080.asm %NAME%.asm
echo - Assembling 8080 version using M80 from Microsoft
..\CPM\simcpm -MA=..\CPM -MB=. -x m80 b:%NAME%,b:%NAME%=b:%NAME%.asm
echo - Linking 8080 version using LINK from DRI
..\CPM\simcpm -MA=..\CPM -MB=. -x link b:%NAME%=b:%NAME%
fc /B TESTZ80.COM %NAME%.COM
if errorlevel 1 echo %NAME% FAILED && exit /b %ERRORLEVEL%

set NAME=TESTZ80R
echo 	MACLIB	Z80>%NAME%.asm
copy /A %NAME%.asm+testz80_8080.asm %NAME%.asm
echo - Assembling 8080 version using RMAC from DRI
..\CPM\simcpm -MA=..\CPM -MB=. -x rmac b:%NAME%.asm
echo - Linking 8080 version using LINK from DRI
..\CPM\simcpm -MA=..\CPM -MB=. -x link b:%NAME%=b:%NAME%
fc /B TESTZ80.COM %NAME%.COM
if errorlevel 1 echo %NAME% FAILED && exit /b %ERRORLEVEL%

del TESTZ80M* TESTZ80R* *.COM *.PRN *.REL *.SYM *_*

echo SUCCESSFUL

goto :eof
