#	$NetBSD: Makefile.inc,v 1.3 2001/11/30 02:26:35 mjl Exp $

SRCS+=	bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
