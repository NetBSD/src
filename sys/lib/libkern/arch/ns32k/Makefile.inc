#	$NetBSD: Makefile.inc,v 1.14 1998/01/21 21:23:26 cgd Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c memchr.S \
	bcmp.S bcopy.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
	strncmp.S strncpy.S htonl.S htons.S ntohl.S ntohs.S \
	scanc.c skpc.c strncasecmp.c __assert.c
