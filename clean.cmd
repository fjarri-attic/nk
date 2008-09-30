:::::::::::
::Main part
:::::::::::
:Main
echo off

call :CleanObjs nkcodec
call :CleanObjs nkfilter
call :CleanObjs nkcontrol

echo *** Cleaning shared ***
cd shared
del /F /Q nkcodec.lib
cd ..

echo *** Cleaning root dir ***
call :CleanLogs

echo on

exit /b

:::::::::::::::::::::::::::::::::
::Clean folders with object files
:::::::::::::::::::::::::::::::::
:CleanObjs
echo *** Cleaning %1 ***
cd %1
set archs=x86 x64 ia64
set wins=wnet wlh
set blds=chk fre
for %%a in (%archs%) do for %%w in (%wins%) do for %%b in (%blds%) do rmdir /S /Q obj%%b_%%w_%%a     
call :CleanLogs
cd ..

exit /b

:::::::::::::::::::::::::::::::::::
::Clean log files and error reports
:::::::::::::::::::::::::::::::::::
:CleanLogs
del /F /Q build*.log
del /F /Q build*.err
del /F /Q build*.wrn

exit /b