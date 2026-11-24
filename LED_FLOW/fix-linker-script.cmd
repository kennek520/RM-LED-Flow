@echo off
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p='STM32F103XX_FLASH.ld'; if(-not (Test-Path $p)){ Write-Host 'ERROR: STM32F103XX_FLASH.ld not found'; exit 1 }; $t=[IO.File]::ReadAllText($p); $n=([regex]::Matches($t,' \(READONLY\)')).Count; if($n -eq 0){ Write-Host 'Nothing to do: no READONLY keyword.' } else { [IO.File]::WriteAllText($p, ($t -replace ' \(READONLY\)',''), (New-Object System.Text.UTF8Encoding($false))); Write-Host ('Fixed: removed ' + $n + ' READONLY keyword(s).') }; pause"
