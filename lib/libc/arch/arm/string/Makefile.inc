# $NetBSD: Makefile.inc,v 1.1.2.2 2002/12/10 06:25:43 thorpej Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S strcmp.S
SRCS+=	strncmp.S memcmp.S
SRCS+=	bcmp.c index.c memchr.c \
	rindex.c strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

