#	$NetBSD: Makefile.inc,v 1.19 2001/12/02 11:03:39 mjl Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.c bzero.c \
	memchr.c memcmp.c memset.c \
	strcat.c strcmp.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c \
	scanc.c skpc.c \
	htonl.c htons.c ntohl.c ntohs.c \
	random.c strtoul.c \
	syncicache.c

SRCS+=	ffs.S strlen.S
