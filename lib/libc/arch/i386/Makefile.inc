#	$NetBSD: Makefile.inc,v 1.9 1998/02/22 07:21:32 mycroft Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcpy.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S \
	htonl.S htons.S ntohl.S ntohs.S
