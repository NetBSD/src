#	$NetBSD: Makefile.inc,v 1.15 1997/10/24 15:32:27 chuck Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c memchr.S \
	bcmp.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.c \
	strncpy.c scanc.S skpc.S htonl.S htons.S ntohl.S ntohs.S \
	strncasecmp.c __assert.c strchr.c
