#	$NetBSD: Makefile.inc,v 1.15 1998/02/22 08:53:33 mycroft Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S \
	strcat.S strcmp.S strcpy.S strlen.S strncasecmp.c strncmp.S strncpy.S \
	scanc.c skpc.c \
	htonl.S htons.S ntohl.S ntohs.S \
	random.S
