#	$NetBSD: Makefile.inc,v 1.16.2.1 2002/08/01 03:27:56 nathanw Exp $

KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S
