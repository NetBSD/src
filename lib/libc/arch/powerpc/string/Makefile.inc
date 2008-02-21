#	$NetBSD: Makefile.inc,v 1.6 2008/02/21 17:37:00 garbled Exp $

SRCS+=	bzero.S ffs.S strlen.S memcmp.S bcopy.S memcpy.S memmove.S

SRCS+=	bcmp.c index.c memchr.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
