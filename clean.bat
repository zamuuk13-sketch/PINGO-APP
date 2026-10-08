@echo off
setlocal

title Pingo App - Clean Build

echo.
echo ========================================
echo       PINGO APP - LIMPAR BUILD
echo ========================================
echo.

if exist build (
    echo [1/2] Removendo pasta build...
    rmdir /S /Q build
    if errorlevel 1 (
        echo [ERRO] Nao foi possivel remover a pasta build.
        pause
        exit /b 1
    )
    echo [OK] build removida.
) else (
    echo [OK] Pasta build nao existe.
)

if exist dist (
    echo [2/2] Removendo pasta dist...
    rmdir /S /Q dist
    if errorlevel 1 (
        echo [ERRO] Nao foi possivel remover a pasta dist.
        pause
        exit /b 1
    )
    echo [OK] dist removida.
) else (
    echo [OK] Pasta dist nao existe.
)

echo.
echo ========================================
echo       LIMPEZA CONCLUIDA!
echo ========================================
echo.
echo Agora voce pode executar o build.bat
echo para compilar o Pingo App novamente.
echo.
pause
endlocal
