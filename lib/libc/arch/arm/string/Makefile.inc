# $NetBSD: Makefile.inc,v 1.1 2000/12/29 20:13:56 bjh21 Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S
SRCS+=	bcmp.c ffs.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

