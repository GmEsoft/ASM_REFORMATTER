@echo off
pushd %~DP0
call mk_reformat.bat
if errorlevel 1 echo MAKE FAILED && exit /b %ERRORLEVEL%
call test.bat
if errorlevel 1 echo TEST FAILED && exit /b %ERRORLEVEL%
echo BUILD SUCCESSFUL
goto :eof
