@echo off
echo Installing DFU Driver...

ver | findstr /c:"Version 5." > nul
if %ERRORLEVEL%==0 goto :install_old

if DEFINED PROCESSOR_ARCHITEW6432 goto :install_new_wow

:install_new
ver | findstr /c:"Version 6.0" > nul
if %ERRORLEVEL%==0 goto :install_new_7
ver | findstr /c:"Version 6.1" > nul
if %ERRORLEVEL%==0 goto :install_new_7
ver | findstr /c:"Version 6.2" > nul
if %ERRORLEVEL%==0 goto :install_new_8
goto :install_new_8_1

:install_new_7
if DEFINED PROCESSOR_ARCHITECTURE goto :install_new_7_64
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win7\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win7\x86\dpinst_x86.exe" /se /sw
exit

:install_new_7_64
if %PROCESSOR_ARCHITECTURE%==x86 goto :install_new_7_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win7\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win7\x64\dpinst_amd64.exe" /se /sw
exit

:install_new_7_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win7\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win7\x86\dpinst_x86.exe" /se /sw
exit

:install_new_8
if DEFINED PROCESSOR_ARCHITECTURE goto :install_new_8_64
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8\x86\dpinst_x86.exe" /se /sw
exit

:install_new_8_64
if %PROCESSOR_ARCHITECTURE%==x86 goto :install_new_8_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8\x64\dpinst_amd64.exe" /se /sw
exit

:install_new_8_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8\x86\dpinst_x86.exe" /se /sw
exit

:install_new_8_1
if DEFINED PROCESSOR_ARCHITECTURE goto :install_new_8_1_64
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8.1\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8.1\x86\dpinst_x86.exe" /se /sw
exit

:install_new_8_1_64
if %PROCESSOR_ARCHITECTURE%==x86 goto :install_new_8_1_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8.1\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8.1\x64\dpinst_amd64.exe" /se /sw
exit

:install_new_8_1_32
rem %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8.1\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8.1\x86\dpinst_x86.exe" /se /sw
exit

:install_new_wow
ver | findstr /c:"Version 6.0" > nul
if %ERRORLEVEL%==0 goto :install_new_wow_7
ver | findstr /c:"Version 6.1" > nul
if %ERRORLEVEL%==0 goto :install_new_wow_7
ver | findstr /c:"Version 6.2" > nul
if %ERRORLEVEL%==0 goto :install_new_wow_8
goto :install_new_wow_8_1

:install_new_wow_7
rem %SystemRoot%\Sysnative\cmd.exe /c %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win7\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win7\x64\dpinst_amd64.exe" /se /sw
exit

:install_new_wow_8
rem %SystemRoot%\Sysnative\cmd.exe /c %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8\x64\dpinst_amd64.exe" /se /sw
exit

:install_new_wow_8_1
rem %SystemRoot%\Sysnative\cmd.exe /c %SystemRoot%\System32\InfDefaultInstall.exe "%~dp0\..\dfuse\Driver\Win8.1\x64\STtube.inf"
"%~dp0\..\dfuse\Driver\Win8.1\x64\dpinst_amd64.exe" /se /sw
exit

:install_old
rem %SystemRoot%\System32\rundll32.exe %SystemRoot%\System32\setupapi.dll,InstallHinfSection DefaultInstall 132 "%~dp0\..\dfuse\Driver\Win7\x86\STtube.inf"
"%~dp0\..\dfuse\Driver\Win7\x86\dpinst_x86.exe" /se /sw
exit
