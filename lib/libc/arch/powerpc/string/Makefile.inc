#	$NetBSD: Makefile.inc,v 1.1.14.2 2002/03/08 21:34:51 nathanw Exp $

SRCS+=	bzero.S ffs.S strlen.S
KMSRCS+=bzero.S ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
