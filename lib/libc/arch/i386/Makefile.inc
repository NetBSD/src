#	$NetBSD: Makefile.inc,v 1.5 1997/10/09 10:07:40 lukem Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
	htonl.S htons.S ntohl.S ntohs.S
