#	$NetBSD: Makefile.inc,v 1.9 1998/02/22 06:57:31 mycroft Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.c ffs.c \
	memchr.c memcpy.c \
	strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.c htons.c ntohl.c ntohs.c \
	random.c
