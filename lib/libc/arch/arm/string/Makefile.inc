# $NetBSD: Makefile.inc,v 1.6 2003/10/14 07:51:45 scw Exp $

SRCS+=  memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S strcmp.S
SRCS+=	strncmp.S memcmp.S
SRCS+=	bcmp.c index.c memchr.c \
	rindex.c strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

