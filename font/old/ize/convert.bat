@echo off
cd /d "%~dp0"
chcp 65001 > nul
echo ==========================================
echo   타자기 전용 한글 폰트 (자모음 완벽 추가본)
echo ==========================================
echo.

echo 1. FontForge 자동화 명령어를 준비합니다...
echo Open("DungGeunMo.ttf") > auto.pe
echo BitmapsAvail([16]) >> auto.pe
echo BitmapsRegen([16]) >> auto.pe
echo Generate("myfont.bdf", "bdf") >> auto.pe

echo.
echo 2. OTF/TTF 폰트를 도트(BDF)로 변환
if exist "C:\Program Files\FontForgeBuilds\fontforge.bat" (
    call "C:\Program Files\FontForgeBuilds\fontforge.bat" -script auto.pe
) else (
    call "C:\Program Files (x86)\FontForgeBuilds\fontforge.bat" -script auto.pe
)

echo.
echo 3. 도트(BDF)를 U8g2 전용 C++ 코드(.h)로 압축
bdfconv.exe -v -f 1 -m "32-128,12593-12643,44032-55203" myfont-16.bdf -o Typewriter_16px.h -n Typewriter_16px
echo.
echo ==========================================
echo   완벽하게 완료되었습니다! 
echo ==========================================
del auto.pe
del myfont-16.bdf
pause
