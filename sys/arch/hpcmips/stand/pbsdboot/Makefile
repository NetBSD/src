#	$NetBSD: Makefile,v 1.3 2000/03/19 11:10:57 takemura Exp $

all: pbsdboot.exe

pbsdboot.exe: pbsdboot.uu
	uudecode pbsdboot.uu
	chmod 755 pbsdboot.exe
	uudecode pbsdboot1.uu
	chmod 755 pbsdboot1.exe

uu::
	if [ X"`strings WMIPSDbg/pbsdboot.exe | grep 'Compiled for WinCE'`" = \
           X'Compiled for WinCE 1.01' ]; then \
          echo This binary is compiled for WinCE 1.01; \
          uuencode WMIPSDbg/pbsdboot.exe pbsdboot1.exe > pbsdboot1.uu; \
        else \
          echo This binary is compiled for WinCE 2.00; \
          uuencode WMIPSDbg/pbsdboot.exe pbsdboot.exe > pbsdboot.uu; \
        fi
