@echo off
chcp 65001 >nul
REM Flip for Fortune - Projeyi GitHub'a yukler.
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

if not exist ".git" (
  git init -b main
)

git config user.name >nul 2>nul || git config user.name "kayra"
git config user.email >nul 2>nul || git config user.email "kayracem01@gmail.com"

git add -A
git commit -m "Flip for Fortune: C++ cekirdek, oda, Coin Collection, yukseltmeler, enchant, AutoFlip, inis efektleri"

git remote get-url origin >nul 2>nul
if errorlevel 1 (
  echo.
  echo GitHub'da BOS bir repo olustur: https://github.com/new
  echo  ^(README / .gitignore EKLEME, Private secebilirsin^)
  echo.
  set /p REPO=Repo adresini yapistir ^(ornek: https://github.com/kullanici/FlipForFortune.git^): 
  call git remote add origin %%REPO%%
)

echo.
echo GitHub'a gonderiliyor... ^(ilk seferde tarayicida GitHub girisi acilabilir^)
git push -u origin main
if errorlevel 1 (
  echo.
  echo HATA: Gonderilemedi. Claude'a bu pencerenin ekran goruntusunu at.
) else (
  echo.
  echo BASARILI! Proje GitHub'da.
)
echo.
pause
