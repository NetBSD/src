#	$NetBSD: Makefile.inc,v 1.9 1997/10/24 15:32:26 chuck Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c bcmp.c ffs.c strcat.c strcmp.c \
	strcpy.c strlen.c strncmp.c strncpy.c scanc.c skpc.c random.c \
	strncasecmp.c __assert.c strchr.c
SRCS+=	divsi3.S htons.S htonl.S ntohs.S ntohl.S
SRCS+=	_memcpy.S memcpy.S bcopy.S memset.S bzero.S memchr.c
