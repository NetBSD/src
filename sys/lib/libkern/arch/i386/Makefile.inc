#	$NetBSD: Makefile.inc,v 1.21 1998/08/04 01:14:36 perry Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.S bswap32.S bswap64.c \
	bcmp.S bzero.S ffs.S \
	memchr.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strncasecmp.c \
	strncmp.c strncpy.c strrchr.S \
	scanc.S skpc.S \
	htonl.S htons.S ntohl.S ntohs.S \
	random.S
