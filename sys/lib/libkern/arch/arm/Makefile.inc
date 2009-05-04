#	$NetBSD: Makefile.inc,v 1.6.108.1 2009/05/04 08:13:50 yamt Exp $

SRCS+=	__assert.c __main.c bswap64.c byte_swap_2.S byte_swap_4.S \
	ffs.S imax.c imin.c lmax.c lmin.c max.c min.c random.c scanc.c \
	skpc.c strcat.c strcasecmp.c \
	strcpy.c strlen.c strncasecmp.c \
	strncpy.c strtoul.c ulmax.c ulmin.c

SRCS+=	divsi3.S clzsi2.S
SRCS+=	memchr.c memcmp.S memcpy.S memset.S memmove.S strcmp.S strncmp.S
