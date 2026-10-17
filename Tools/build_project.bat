@echo off
REM Flip for Fortune - C++ kodunu derler. Once Unreal Editor'u KAPAT.
set UPROJECT=%~dp0..\MyProject.uproject
set LOG=%~dp0..\Saved\Logs\build_output.txt
echo Derleniyor... (birkac dakika surebilir)
call "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" MyProjectEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -NoHotReloadFromIDE > "%LOG%" 2>&1
if %ERRORLEVEL%==0 (
  echo.
  echo BASARILI! Simdi MyProject.uproject'i acabilirsin.
) else (
  echo.
  echo HATA! Claude'a "derleme hata verdi" yaz, log'a kendisi bakacak.
)
echo.
pause
