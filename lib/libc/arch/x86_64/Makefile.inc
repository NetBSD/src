#	$NetBSD: Makefile.inc,v 1.3 2003/10/06 22:57:23 fvdl Exp $

KMSRCS=	bcmp.S bcopy.S bzero.S ffs.S index.S rindex.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S
