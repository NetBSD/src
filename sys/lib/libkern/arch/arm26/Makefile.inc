#	$NetBSD: Makefile.inc,v 1.1 2000/05/09 21:56:05 bjh21 Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	byte_swap_2.S byte_swap_4.S bswap64.c bcmp.c ffs.c strcat.c strcmp.c \
	strcpy.c strlen.c strncmp.c strncpy.c scanc.c skpc.c random.c \
	strncasecmp.c __assert.c
SRCS+=	divsi3.S
SRCS+=	memchr.c memcmp.c _memcpy.S memcpy.S memmove.S memset.S
SRCS+=	bcopy.S bzero.S
