#	$NetBSD: Makefile.inc,v 1.15 2000/12/13 07:05:55 enami Exp $

KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S

SRCS+=	__syscall.S
