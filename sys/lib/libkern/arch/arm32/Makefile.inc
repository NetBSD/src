#	$NetBSD: Makefile.inc,v 1.11 1998/08/04 03:27:32 perry Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap16.c bswap32.c bswap64.c bcmp.c ffs.c strcat.c strcmp.c \
	strcpy.c strlen.c strncmp.c strncpy.c scanc.c skpc.c random.c \
	strncasecmp.c __assert.c
SRCS+=	divsi3.S htons.S htonl.S ntohs.S ntohl.S
SRCS+=	memchr.c memcmp.c _memcpy.S memcpy.S memmove.S memset.S
SRCS+=	bcopy.S bzero.S
