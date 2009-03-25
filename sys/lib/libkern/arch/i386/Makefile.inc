#	$NetBSD: Makefile.inc,v 1.28 2009/03/25 01:26:13 darran Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	byte_swap_2.S byte_swap_4.S bswap64.c ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcasecmp.c \
	strcpy.S strlen.S strncasecmp.c \
	strncmp.c strncpy.c strtoul.c strrchr.S \
	scanc.S skpc.S \
	random.S crc32.c
