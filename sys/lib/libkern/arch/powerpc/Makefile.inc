#	$NetBSD: Makefile.inc,v 1.13.8.1 1999/06/21 01:25:31 thorpej Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.c ffs.c \
	memchr.c memcmp.c memset.c \
	strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.c htons.c ntohl.c ntohs.c \
	random.c \
	syncicache.c
