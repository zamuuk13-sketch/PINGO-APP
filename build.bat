@echo off
setlocal

title Pingo App - Build

echo.
echo ========================================
echo        PINGO APP - BUILD WINDOWS
echo ========================================
echo.

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERRO] g++ nao foi encontrado.
    echo Instale MinGW-w64 e adicione o bin ao PATH.
    echo.
    pause
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERRO] CMake nao foi encontrado.
    echo Instale o CMake e adicione-o ao PATH.
    echo.
    pause
    exit /b 1
)

echo [OK] g++ encontrado:
g++ --version | findstr /B /C:"g++"
echo.
echo [OK] CMake encontrado:
cmake --version | findstr /B /C:"cmake version"
echo.

if not exist build mkdir build
if not exist dist mkdir dist

echo [1/2] Configurando projeto com CMake...
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 (
    echo.
    echo [ERRO] Falha ao configurar o projeto.
    pause
    exit /b 1
)

echo.
echo [2/2] Compilando Pingo App em Release...
cmake --build build --config Release -j
if errorlevel 1 (
    echo.
    echo [ERRO] Falha na compilacao.
    pause
    exit /b 1
)

if not exist build\PingoApp.exe (
    echo.
    echo [ERRO] A compilacao terminou, mas PingoApp.exe nao foi encontrado.
    pause
    exit /b 1
)

copy /Y "build\PingoApp.exe" "dist\PingoApp.exe" >nul
if errorlevel 1 (
    echo.
    echo [ERRO] Nao foi possivel copiar o executavel para dist.
    pause
    exit /b 1
)

echo.
echo ========================================
echo       COMPILACAO CONCLUIDA!
echo ========================================
echo.
echo Executavel:
echo %CD%\dist\PingoApp.exe
echo.
echo Agora voce pode executar o PingoApp.exe.
echo.
pause
endlocal
