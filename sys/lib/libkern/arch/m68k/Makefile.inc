#	$NetBSD: Makefile.inc,v 1.12.8.1 1997/10/14 10:27:36 thorpej Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.S \
	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.S \
	strncpy.S htonl.S htons.S ntohl.S ntohs.S scanc.S skpc.S \
	strncasecmp.c __assert.c strchr.c
