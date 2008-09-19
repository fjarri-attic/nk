echo off

echo *** Building nkcodec ***
cd nkcodec
build
copy obj%BUILD_ALT_DIR%\i386\nkcodec.lib ..\shared
cd ..

echo *** Building nkformatting ***
cd nkformatting
build
copy obj%BUILD_ALT_DIR%\i386\nkformatting.lib ..\shared
cd ..

echo *** Building nkfilter ***
cd nkfilter
build
copy obj%BUILD_ALT_DIR%\i386\nkfilter.sys ..\compiled
cd ..

echo *** Building nkcontrol ***
cd nkcontrol
build
copy obj%BUILD_ALT_DIR%\i386\nkcontrol.exe ..\compiled
cd ..

echo on

