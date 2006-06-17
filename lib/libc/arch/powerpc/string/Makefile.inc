#	$NetBSD: Makefile.inc,v 1.5 2006/06/17 18:04:23 uwe Exp $

SRCS+=	bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
