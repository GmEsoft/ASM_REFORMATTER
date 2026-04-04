@echo off
cd %~dp0
call vcvars32.bat
if errorlevel 1 exit /b %ERRORLEVEL%
set cmnd=cl reformat.c reformatter.c convz80.c conv8080.c /Fe:reformat.exe %*
echo %cmnd%
%cmnd%
if errorlevel 1 exit /B %ERRORLEVEL%
del *.obj
echo MAKE SUCCESSFUL
goto :eof
