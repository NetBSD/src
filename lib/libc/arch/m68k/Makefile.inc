#	$NetBSD: Makefile.inc,v 1.4 1996/06/01 01:08:02 jtc Exp $

KMINCLUDES=	arch/m68k/DEFS.h arch/m68k/SYS.h
KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.S \
	strncpy.S htonl.S htons.S ntohl.S ntohs.S \
	ashldi3.S ashrdi3.S lshrdi3.S
