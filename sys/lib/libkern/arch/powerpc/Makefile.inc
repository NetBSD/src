#	$NetBSD: Makefile.inc,v 1.11 1998/08/04 03:37:03 perry Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.c ffs.c \
	memchr.c memcmp.c memcpy.c memmove.c memset.c \
	strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.c htons.c ntohl.c ntohs.c \
	random.c
