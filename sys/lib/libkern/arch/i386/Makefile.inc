#	$NetBSD: Makefile.inc,v 1.13.8.1 1997/10/14 10:27:25 thorpej Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c \
	bcmp.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.c \
	strncpy.c scanc.S skpc.S htonl.S htons.S ntohl.S ntohs.S \
	strncasecmp.c __assert.c strchr.c
