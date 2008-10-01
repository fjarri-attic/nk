echo off

set BASEDIR=c:\DDK

echo *** Building nkcodec ***
call ddkbuild WLHNet free nkcodec
copy /Y nkcodec\objfre_wnet_x86\i386\nkcodec.lib shared

echo *** Building nkcontrol ***
call ddkbuild WLHNet free nkcontrol

echo *** Building nkfilter ***
call ddkbuild WLHNet checked nkfilter

echo *** Copying files ***
xcopy /Y nkfilter\objchk_wnet_x86\i386\nkfilter.sys compiled\*
xcopy /Y nkcontrol\objfre_wnet_x86\i386\nkcontrol.exe compiled\*
xcopy /Y inf\nkfilter.inf compiled\*

echo on
