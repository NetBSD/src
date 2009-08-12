#	$NetBSD: Makefile.inc,v 1.8 2009/08/12 21:20:40 dsl Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S
SRCS+=	ffs.S
SRCS+=	divsi3.S clzsi2.S
SRCS+=	memcmp.S memcpy.S memset.S memmove.S strcmp.S strncmp.S

SRCS+=	__assert.c __main.c bswap64.c
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c random.c scanc.c
SRCS+=	skpc.c strcat.c strcasecmp.c
SRCS+=	strcpy.c strlen.c strncasecmp.c
SRCS+=	strncpy.c strtoul.c ulmax.c ulmin.c

SRCS+=	memchr.c
