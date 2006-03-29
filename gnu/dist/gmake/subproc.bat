cd w32\subproc
set MAKE=%2
set MAKEFILE=%1
if x%2 == x set MAKE=nmake
%MAKE% /f %MAKEFILE%
cd ..\..
