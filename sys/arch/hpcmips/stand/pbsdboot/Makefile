#	$NetBSD: Makefile,v 1.1.1.1 1999/09/16 12:23:30 takemura Exp $

all: pbsdboot.exe

pbsdboot.exe: pbsdboot.uu
	uudecode pbsdboot.uu

uu: WMIPSDbg/pbsdboot.exe
	uuencode WMIPSDbg/pbsdboot.exe pbsdboot.exe > pbsdboot.uu
