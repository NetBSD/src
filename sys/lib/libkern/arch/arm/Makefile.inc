#	$NetBSD: Makefile.inc,v 1.1.4.4 2002/12/11 06:46:24 thorpej Exp $

SRCS+=	__assert.c __main.c bswap64.c byte_swap_2.S byte_swap_4.S \
	ffs.S imax.c imin.c lmax.c lmin.c max.c min.c random.c scanc.c \
	skpc.c strcat.c strcasecmp.c \
	strcpy.c strlen.c strncasecmp.c \
	strncpy.c strtoul.c ulmax.c ulmin.c

SRCS+=	divsi3.S
SRCS+=	memchr.c memcmp.S memcpy.S memset.S memmove.S strcmp.S strncmp.S
