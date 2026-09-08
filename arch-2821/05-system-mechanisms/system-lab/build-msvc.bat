@echo off
setlocal

cl /nologo /W4 /Z7 /Od system_lab.c /Fe:system_lab.exe /link /DEBUG /PDB:system_lab.pdb
if errorlevel 1 exit /b 1

echo Built system_lab.exe and system_lab.pdb
