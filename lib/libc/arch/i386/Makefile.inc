#	$NetBSD: Makefile.inc,v 1.11 1998/08/04 01:35:57 perry Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S \
	htonl.S htons.S ntohl.S ntohs.S
