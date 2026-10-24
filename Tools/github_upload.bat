@echo off
chcp 65001 >nul
REM Flip for Fortune - Projeyi GitHub'a yukler. (Content/ klasoru yuklenmez: .gitignore)
cd /d "%~dp0.."

where git >nul 2>nul
if errorlevel 1 (
  echo Git bulunamadi. Kuruluyor...
  winget install --id Git.Git -e --accept-source-agreements --accept-package-agreements
  echo.
  echo Git kuruldu. Bu pencereyi kapatip dosyayi TEKRAR calistir.
  pause
  exit /b
)

if not exist ".git" git init -b main

git config user.name >nul 2>nul || git config user.name "kayra"
git config user.email >nul 2>nul || git config user.email "kayracem01@gmail.com"

REM Takilmis kilit dosyasi varsa temizle (onceki yarim kalan islem)
if exist ".git\index.lock" del /f /q ".git\index.lock"

for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyy-MM-dd"') do set TODAY=%%i
echo.
echo Bu yuklemede ne degisti? ^(bos birakirsan: "Guncelleme %TODAY%"^)
set "MSG="
set /p MSG=Commit mesaji: 
if "%MSG%"=="" set "MSG=Guncelleme %TODAY%"

REM Gonderilemeyen eski commitleri (buyuk harita dosyasi iceren) tek committe birlestir
git fetch origin >nul 2>nul
git rev-parse --verify origin/main >nul 2>nul
if not errorlevel 1 git reset --soft origin/main

REM Content klasoru artik takip edilmiyor (dosyalar bilgisayarinda kalir, sadece GitHub'a gitmez)
git rm -r -q --cached Content >nul 2>nul

git add -A
git commit -m "%TODAY%: %MSG%"

git remote get-url origin >nul 2>nul
if errorlevel 1 git remote add origin https://github.com/KayraCem/FlipForFortune.git

echo.
echo GitHub'a gonderiliyor... ^(ilk seferde tarayicida GitHub girisi acilabilir^)
git push -u origin main
if errorlevel 1 (
  echo.
  echo HATA: Gonderilemedi. Bu pencerenin ekran goruntusunu Claude'a at.
) else (
  echo.
  echo BASARILI! Proje GitHub'da: https://github.com/KayraCem/FlipForFortune
)
echo.
pause
