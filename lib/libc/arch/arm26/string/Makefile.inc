# $NetBSD: Makefile.inc,v 1.1 2000/05/09 21:55:50 bjh21 Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S
SRCS+=	bcmp.c ffs.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

