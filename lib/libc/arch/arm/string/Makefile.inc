# $NetBSD: Makefile.inc,v 1.1.2.1 2001/11/14 19:31:51 nathanw Exp $

SRCS+=  memcpy.S _memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S
SRCS+=	bcmp.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

