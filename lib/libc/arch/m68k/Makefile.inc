#	$NetBSD: Makefile.inc,v 1.2 1995/12/13 22:21:33 thorpej Exp $

KMINCLUDES=	arch/m68k/DEFS.h arch/m68k/SYS.h
KMSRCS=	bcmp.S bzero.S ffs.S strcat.c strcmp.S strcpy.S strlen.S strncmp.S \
	strncpy.S htonl.S htons.S ntohl.S ntohs.S
