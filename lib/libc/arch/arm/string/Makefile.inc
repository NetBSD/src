# $NetBSD: Makefile.inc,v 1.4 2002/11/23 03:10:27 chris Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S strcmp.S
SRCS+=	strncmp.S
SRCS+=	bcmp.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

