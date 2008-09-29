echo off

echo *** Building nkcodec ***
cd nkcodec
build
xcopy /Y obj%BUILD_ALT_DIR%\i386\nkcodec.lib ..\shared\*
cd ..

echo *** Building nkfilter ***
cd nkfilter
build
xcopy /Y obj%BUILD_ALT_DIR%\i386\nkfilter.sys ..\compiled\*
cd ..

echo *** Building nkcontrol ***
cd nkcontrol
build
xcopy /Y obj%BUILD_ALT_DIR%\i386\nkcontrol.exe ..\compiled\*
cd ..

xcopy /Y inf\nkfilter.inf compiled\*

echo on
