#	$NetBSD: Makefile,v 1.2 1999/12/04 02:43:48 takemura Exp $

all: pbsdboot.exe

pbsdboot.exe: pbsdboot.uu
	uudecode pbsdboot.uu
	chmod 755 pbsdboot.exe

uu: WMIPSDbg/pbsdboot.exe
	uuencode WMIPSDbg/pbsdboot.exe pbsdboot.exe > pbsdboot.uu
