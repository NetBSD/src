#	$NetBSD: Makefile,v 1.4.6.1 2002/02/11 20:08:09 jdolecek Exp $

UUDECODE?=	uudecode

all: pbsdboot.exe pbsdboot1.exe

pbsdboot.exe: pbsdboot.uu
	${UUDECODE} pbsdboot.uu
	chmod 755 pbsdboot.exe

pbsdboot1.exe: pbsdboot1.uu
	${UUDECODE} pbsdboot1.uu
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
