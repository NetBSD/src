#	$NetBSD: Makefile,v 1.1.1.1.8.1 1999/12/27 18:32:07 wrstuden Exp $

all: pbsdboot.exe

pbsdboot.exe: pbsdboot.uu
	uudecode pbsdboot.uu
	chmod 755 pbsdboot.exe

uu: WMIPSDbg/pbsdboot.exe
	uuencode WMIPSDbg/pbsdboot.exe pbsdboot.exe > pbsdboot.uu
