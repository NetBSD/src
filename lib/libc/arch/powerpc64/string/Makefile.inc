#	$NetBSD: Makefile.inc,v 1.2 2006/07/07 04:54:52 ross Exp $

SRCS+=	bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
