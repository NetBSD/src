#	$NetBSD: Makefile.inc,v 1.13 1999/11/29 23:12:58 simonb Exp $

SRCS+=	__assert.c __main.c bcmp.c bswap64.c byte_swap_2.S byte_swap_4.S \
	ffs.c imax.c imin.c lmax.c lmin.c max.c min.c random.c scanc.c \
	skpc.c strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c \
	strncpy.c strtoul.c ulmax.c ulmin.c

SRCS+=	divsi3.S
SRCS+=	memchr.c memcmp.c _memcpy.S memcpy.S memmove.S memset.S
SRCS+=	bcopy.S bzero.S
