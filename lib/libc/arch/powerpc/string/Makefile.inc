#	$NetBSD: Makefile.inc,v 1.4 2002/02/20 22:45:05 eeh Exp $

SRCS+=	bzero.S ffs.S strlen.S
KMSRCS+=bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
