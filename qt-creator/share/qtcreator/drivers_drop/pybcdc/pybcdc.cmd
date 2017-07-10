@echo off
echo Installing Python Board Driver...

ver | findstr /c:"Version 5." > nul
if %ERRORLEVEL%==0 goto :install_old

if DEFINED PROCESSOR_ARCHITEW6432 goto :install_new_wow

:install_new
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\pybcdc.inf"
if DEFINED PROCESSOR_ARCHITECTURE goto :install_new_64
"%~dp0\dpinst_x86.exe" /se /sw
exit

:install_new_64
if %PROCESSOR_ARCHITECTURE%==x86 goto :install_new_32
"%~dp0\dpinst_amd64.exe" /se /sw
exit

:install_new_32
"%~dp0\dpinst_x86.exe" /se /sw
exit

:install_new_wow
rem %SystemRoot%\Sysnative\cmd.exe /c %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\pybcdc.inf"
"%~dp0\dpinst_amd64.exe" /se /sw
exit

:install_old
rem %SystemRoot%\System32\rundll32.exe %SystemRoot%\System32\setupapi.dll,InstallHinfSection DefaultInstall 132 "%~dp0\pybcdc.inf"
"%~dp0\dpinst_x86.exe" /se /sw
exit
