#	$NetBSD: Makefile.inc,v 1.4 2009/08/12 21:20:41 dsl Exp $

SRCS+=	byte_swap_2.S byte_swap_4.S
SRCS+=	ffs.S
SRCS+=	memchr.S memcmp.S memcpy.S memmove.S memset.S
SRCS+=	strcat.S strchr.S strcmp.S
SRCS+=	strcpy.S strlen.S
SRCS+=	strrchr.S
SRCS+=	scanc.S skpc.S
SRCS+=	random.S

SRCS+=	__main.c __assert.c
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	bswap64.c
SRCS+=	strcasecmp.c
SRCS+=	strncasecmp.c
SRCS+=	strncmp.c strncpy.c strtoul.c
