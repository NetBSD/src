#	$NetBSD: Makefile.inc,v 1.6 1998/02/22 06:37:54 mycroft Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S memcpy.S memset.S \
	strcat.S strcmp.S strcpy.S strlen.S \
	htonl.S htons.S ntohl.S ntohs.S
