@echo off
:: build.bat - build WunderlandLE and package it as an unsigned .sis
::
:: Usage (from an SDK command prompt, project on the same drive as the SDK):
::   build.bat                 GCCE UREL with abld   (S60 3rd/5th Edition SDKs)  <- release builds
::   build.bat sbs             GCCE UREL with Raptor (Symbian^3 / Qt SDK)        <- compile check only
::   build.bat clean
::
:: Requires EPOCROOT (e.g. \Symbian\9.1\S60_3rd\) - set by "devices -setdefault" or the SDK env.
setlocal
set ROOT=%~dp0
if "%EPOCROOT%"=="" (
    echo EPOCROOT is not set. Run from an SDK command prompt or "devices -setdefault @..." first.
    exit /b 1
)

if /i "%1"=="clean" goto clean
if /i "%1"=="sbs" goto sbs

:abld
set PLATFORM=gcce
pushd "%ROOT%group"
call bldmake bldfiles || goto fail
call .\abld.bat build gcce urel || goto fail
popd
goto package

:sbs
set PLATFORM=armv5
pushd "%ROOT%group"
call sbs -c arm.v5.urel.gcce4_4_1 || goto fail
popd
goto package

:package
set TARGET=urel
pushd "%ROOT%sis"
:: Substitute the Carbide.c++ variables so plain makesis can read the PKG.
perl -pe "s/\$\(EPOCROOT\)/$ENV{EPOCROOT}/gi; s/\$\(PLATFORM\)/$ENV{PLATFORM}/gi; s/\$\(TARGET\)/$ENV{TARGET}/gi" WunderlandLE.pkg > WunderlandLE_resolved.pkg || goto fail
makesis WunderlandLE_resolved.pkg WunderlandLE.sis || goto fail
del WunderlandLE_resolved.pkg
popd
echo.
echo Done: %ROOT%sis\WunderlandLE.sis (unsigned)
exit /b 0

:clean
pushd "%ROOT%group"
if exist abld.bat call .\abld.bat reallyclean
call bldmake clean
popd
del /q "%ROOT%sis\*.sis" 2>nul
exit /b 0

:fail
echo BUILD FAILED
exit /b 1
