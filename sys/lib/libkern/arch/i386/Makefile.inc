#	$NetBSD: Makefile.inc,v 1.19 1998/02/22 07:03:08 mycroft Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c \
	bcmp.S bzero.S ffs.S \
	memchr.S memcpy.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strncasecmp.c strncmp.c strncpy.c strrchr.S \
	scanc.S skpc.S \
	htonl.S htons.S ntohl.S ntohs.S \
	random.c
