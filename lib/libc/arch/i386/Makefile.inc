#	$NetBSD: Makefile.inc,v 1.18 2003/09/06 22:56:47 christos Exp $

KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S
