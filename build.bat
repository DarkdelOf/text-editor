@echo off
echo Compilando..
gcc main.c -o app.exe -I include -L lib -lraylib -lopengl32 -lgdi32 -lwinmm
if %errorlevel% neq 0 exit /b %errorlevel%
echo Sucesso!
app.exe
