#	$NetBSD: Makefile.inc,v 1.17 1998/02/22 06:08:37 mycroft Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c memchr.S memset.c \
	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S strncmp.c \
	strncpy.c scanc.S skpc.S htonl.S htons.S ntohl.S ntohs.S \
	random.c strncasecmp.c __assert.c
