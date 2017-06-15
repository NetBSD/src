@echo off
set ext=
set file=

:loop
@set arg=%1
if "%arg%" == "" goto end
if "%arg:~0,1%" == "+" goto next
if "%arg%" == "-t" goto next
if "%arg%" == "ds" goto ds
if "%arg%" == "DS" goto ds
if "%arg%" == "dlv" goto dlv
if "%arg%" == "DLV" goto dlv
if "%arg%" == "dnskey" goto dnskey
if "%arg%" == "DNSKEY" goto dnskey
set file=%arg%
goto next

:ds
set ext=ds
goto next

:dlv
set ext=dlv
goto next

:dnskey
set ext=dnskey
goto next

:next
shift
goto loop

:end

set name=%file%.%ext%.db
type %name%
