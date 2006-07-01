#	$NetBSD: Makefile.inc,v 1.1 2006/07/01 16:37:20 ross Exp $

SRCS+=	ffs.S strlen.S
KMSRCS+=ffs.S strlen.S

SRCS+=	bcmp.c bcopy.c index.c memchr.c memcmp.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
