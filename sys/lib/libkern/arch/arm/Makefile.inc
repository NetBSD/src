#	$NetBSD: Makefile.inc,v 1.1.4.1 2001/08/24 00:11:50 nathanw Exp $

SRCS+=	__assert.c __main.c bswap64.c byte_swap_2.S byte_swap_4.S \
	ffs.c imax.c imin.c lmax.c lmin.c max.c min.c random.c scanc.c \
	skpc.c strcat.c strcmp.c strcasecmp.c \
	strcpy.c strlen.c strncasecmp.c strncmp.c \
	strncpy.c strtoul.c ulmax.c ulmin.c

SRCS+=	divsi3.S
SRCS+=	memchr.c memcmp.c _memcpy.S memcpy.S memmove.S memset.S
