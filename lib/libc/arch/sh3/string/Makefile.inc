#	$NetBSD: Makefile.inc,v 1.1.6.2 2002/12/10 06:25:44 thorpej Exp $

SRCS+=	bcmp.c bcopy.S bzero.S ffs.S index.c memchr.c memcmp.c memset.S \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
SRCS+=	memcpy.S memmove.S
