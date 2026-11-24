@echo off
setlocal
pushd "%~dp0"

echo ============================================
echo  STM32F103 LED_FLOW  -  build and flash
echo ============================================
echo.

echo [1/3] Building...
"D:\CLion 2026.2.2\bin\cmake\win\x64\bin\cmake.exe" --build --target LED_FLOW --preset Debug
if errorlevel 1 goto BUILDFAIL

echo.
echo [2/3] Patching linker script for GCC 10...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p='STM32F103XX_FLASH.ld'; $t=[IO.File]::ReadAllText($p); $n=([regex]::Matches($t,' \(READONLY\)')).Count; if($n -gt 0){ [IO.File]::WriteAllText($p, ($t -replace ' \(READONLY\)',''), (New-Object System.Text.UTF8Encoding($false))); Write-Host ('      patched ' + $n + ' READONLY keyword(s)') } else { Write-Host '      nothing to patch' }"

echo.
echo [3/3] Flashing...
"D:\OpenOCD\bin\openocd.exe" -f stm32f1.cfg -c "program build/Debug/LED_FLOW.elf verify reset exit"
if errorlevel 1 goto FLASHFAIL

echo.
echo [OK] Verified OK - the program is running now.
goto END

:BUILDFAIL
echo.
echo [FAIL] Build error, read the messages above.
goto END

:FLASHFAIL
echo.
echo [FAIL] Flash failed. Check these:
echo   1. ST-Link plugged into USB
echo   2. 3.3V / G / DIO / CLK wires connected
echo   3. PWR LED on the blue pill is lit
echo   4. ST-Link driver installed (WinUSB)
goto END

:END
echo.
popd
if "%~1"=="" pause
