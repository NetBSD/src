# $NetBSD: Makefile.inc,v 1.2 1997/10/06 00:40:31 mark Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memset.S bzero.S
SRCS+=	bcmp.c ffs.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

