@echo off
cd %~dp0

pushd TEST_CONV

..\reformat -i:test8080.asm -o:test8080_Z80.asm -xz -xn
..\reformat -i:test8080_Z80.asm -o:test8080_8080.asm -x8 -xn

..\reformat -i:testz80.asm -o:testz80_8080.asm -x8 -xn -xl
..\reformat -i:testz80_8080.asm -o:testz80_z80.asm -xz -xn -xl

fc test8080.asm test8080_8080.asm
if errorlevel 1 echo test8080.asm FAILED && exit /b %ERRORLEVEL%

fc testz80.asm testz80_z80.asm
if errorlevel 1 echo testz80.asm FAILED && exit /b %ERRORLEVEL%

echo TEST SUCCESSFUL

goto :eof
