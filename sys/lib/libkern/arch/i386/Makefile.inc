#	$NetBSD: Makefile.inc,v 1.18 1998/02/22 06:40:17 mycroft Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c \
	bcmp.S bzero.S ffs.S \
	memchr.S memcpy.S memset.S \
	strcat.S strcmp.S strcpy.S strlen.S strncasecmp.c strncmp.c strncpy.c \
	scanc.S skpc.S \
	htonl.S htons.S ntohl.S ntohs.S \
	random.c
