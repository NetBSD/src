# $NetBSD: Makefile.inc,v 1.3 2002/11/16 18:28:52 rearnsha Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S strcmp.S
SRCS+=	bcmp.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

