echo off

echo *** Cleaning nkcodec ***

cd nkcodec
rmdir /S /Q objchk_wnet_x86
del /F /Q build*.log
del /F /Q build*.err
del /F /Q build*.wrn
cd ..

echo *** Cleaning nkformatting ***

cd nkformatting
rmdir /S /Q objchk_wnet_x86
del /F /Q build*.log
del /F /Q build*.err
del /F /Q build*.wrn
cd ..

echo *** Cleaning nkfilter ***

cd nkfilter
rmdir /S /Q objchk_wnet_x86
del /F /Q build*.log
del /F /Q build*.err
del /F /Q build*.wrn
cd ..

echo *** Cleaning nkcontrol ***

cd nkcontrol
rmdir /S /Q objchk_wnet_x86
del /F /Q build*.log
del /F /Q build*.err
del /F /Q build*.wrn
cd ..

echo *** Cleaning shared ***

cd shared
del /F /Q nkcodec.lib
del /F /Q nkformatting.lib
cd ..

echo on
