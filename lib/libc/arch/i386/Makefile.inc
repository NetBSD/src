#	$NetBSD: Makefile.inc,v 1.12 1999/01/15 13:31:17 bouyer Exp $

KMINCLUDES= arch/i386/SYS.h
KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S
