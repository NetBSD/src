#	$NetBSD: Makefile.inc,v 1.6.8.1 1997/10/14 10:27:03 thorpej Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c bcmp.c ffs.c strcat.c strcmp.c \
	strcpy.c strlen.c strncmp.c strncpy.c scanc.c skpc.c random.c \
	strncasecmp.c __assert.c strchr.c
SRCS+=	divsi3.S htons.S htonl.S ntohs.S ntohl.S
SRCS+=	_memcpy.S memcpy.S bcopy.S memset.S bzero.S
