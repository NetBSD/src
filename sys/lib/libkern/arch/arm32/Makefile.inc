#	$NetBSD: Makefile.inc,v 1.10 1998/01/21 21:23:22 cgd Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c bcmp.c ffs.c strcat.c strcmp.c \
	strcpy.c strlen.c strncmp.c strncpy.c scanc.c skpc.c random.c \
	strncasecmp.c __assert.c
SRCS+=	divsi3.S htons.S htonl.S ntohs.S ntohl.S
SRCS+=	_memcpy.S memcpy.S bcopy.S memset.S bzero.S memchr.c
