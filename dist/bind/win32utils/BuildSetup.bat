echo off
rem
rem Copyright (C) 2004,2005  Internet Systems Consortium, Inc. ("ISC")
rem Copyright (C) 2001-2002  Internet Software Consortium.
rem 
rem Permission to use, copy, modify, and distribute this software for any
rem purpose with or without fee is hereby granted, provided that the above
rem copyright notice and this permission notice appear in all copies.
rem 
rem THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
rem REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
rem AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
rem INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
rem LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
rem OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
rem PERFORMANCE OF THIS SOFTWARE.

rem BuildSetup.bat
rem This script sets up the files necessary ready to build BIND 9.
rem This requires perl to be installed on the system.

rem Set up the configuration file
cd ..
copy config.h.win32 config.h
cd win32utils

rem Generate the version information
perl makeversion.pl

rem Generate header files for lib/dns

call dnsheadergen.bat

echo Ensure that the OpenSSL sources are at the same level in
echo the directory tree and is named openssl-0.9.8d or libdns
echo will not build. 

rem Make sure that the Build directories are there.

if NOT Exist ..\Build mkdir ..\Build
if NOT Exist ..\Build\Release mkdir ..\Build\Release
if NOT Exist ..\Build\Debug mkdir ..\Build\Debug

echo Copying the ARM and the Installation Notes.

copy ..\COPYRIGHT ..\Build\Release
copy ..\README ..\Build\Release
copy readme1st.txt ..\Build\Release
copy index.html ..\Build\Release
copy ..\doc\arm\*.html ..\Build\Release
copy ..\doc\arm\Bv9ARM.pdf ..\Build\Release
copy ..\CHANGES ..\Build\Release
copy ..\FAQ ..\Build\Release

echo Copying the OpenSSL DLL.

copy ..\..\openssl-0.9.8d\out32dll\libeay32.dll ..\Build\Release\
copy ..\..\openssl-0.9.8d\out32dll\libeay32.dll ..\Build\Debug\

rem Done
