@echo off
REM AVISO: O compilador sendo usado é o w64devkit na pasta C:

echo Compilando com w64devkit...

set PATH=C:\w64devkit\bin;%PATH%

gcc main.c -o app.exe -I include -L lib -lraylib -lopengl32 -lgdi32 -lwinmm

if %errorlevel% neq 0 (
    pause
    exit /b %errorlevel%
)

echo Sucesso! Rodando o programa...
app.exe