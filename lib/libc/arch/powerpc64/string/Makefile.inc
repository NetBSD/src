#	$NetBSD: Makefile.inc,v 1.3 2009/07/18 09:44:30 dsl Exp $

SRCS+=	bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c memchr.c memcmp.c \
	strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
