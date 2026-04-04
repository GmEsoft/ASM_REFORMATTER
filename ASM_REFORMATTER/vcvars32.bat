@echo off
echo :: Set Visual Studio environment ::

if defined VS140COMNTOOLS (
  call :vcvars32 "%VS140COMNTOOLS%"
) else (
  echo No VSxxxCOMNTOOLS found!
  exit /b 1
)
goto :eof

:vcvars32
set DIR=%~1\..\..
for %%D in ( "%DIR%" ) do set DIR=%%~fD
set VCVARS32="%DIR%\VC\bin\vcvars32.bat"
echo VS found in=%DIR%
if not exist %VCVARS32% echo NOT FOUND: %VCVARS32% && exit /B 1
call %VCVARS32%
goto :eof
