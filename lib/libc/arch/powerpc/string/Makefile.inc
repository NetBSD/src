#	$NetBSD: Makefile.inc,v 1.2 2001/11/25 01:09:59 mjl Exp $

SRCS+=	bzero.S ffs.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
