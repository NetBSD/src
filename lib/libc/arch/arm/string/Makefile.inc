# $NetBSD: Makefile.inc,v 1.7 2009/07/18 09:44:30 dsl Exp $

SRCS+=  memcpy.S bcopy.S memmove.S memset.S bzero.S ffs.S strcmp.S
SRCS+=	strncmp.S memcmp.S
SRCS+=	bcmp.c memchr.c \
	strcat.c strcpy.c strcspn.c strlen.c \
	strncat.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c

