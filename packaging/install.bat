@echo off
chcp 65001 >nul
setlocal

rem Folder "Common Files\VST3" needs admin rights: re-launch elevated if needed.
net session >nul 2>&1
if %errorlevel% neq 0 (
    powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

cd /d "%~dp0"

set "NAME=ShaVeRMA Hit Maker"
set "DEST=%CommonProgramFiles%\VST3"

echo.
echo   Ставим шаВеRMA Hit Maker...
echo.

if not exist "%DEST%" mkdir "%DEST%"
if exist "%DEST%\%NAME%.vst3" rmdir /s /q "%DEST%\%NAME%.vst3"

xcopy "%NAME%.vst3" "%DEST%\%NAME%.vst3\" /E /I /Y /Q >nul
if %errorlevel% neq 0 (
    echo   Не получилось скопировать. Распакуй архив и запусти install.bat из папки.
    pause
    exit /b 1
)

echo   Готово! Шаурма в лаваше.
echo.
echo   Дальше в Ableton:
echo     1. Options - Preferences - Plug-Ins - включи "Use VST3 Plug-In System Folders"
echo     2. Нажми "Rescan"
echo     3. Ищи "ShaVeRMA Hit Maker" в Plug-Ins - Melody Makers
echo     4. Кидай на мастер и жми большую красную кнопку
echo.
pause
