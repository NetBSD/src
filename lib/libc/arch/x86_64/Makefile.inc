#	$NetBSD: Makefile.inc,v 1.4 2004/03/25 14:21:29 drochner Exp $

KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S
