#	$NetBSD: Makefile.inc,v 1.17 1998/08/04 03:35:59 perry Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c \
	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strncasecmp.c strncmp.S strncpy.S \
	scanc.c skpc.c \
	htonl.S htons.S ntohl.S ntohs.S \
	random.S
