#	$NetBSD: Makefile.inc,v 1.2.124.1 2009/05/13 17:22:15 jym Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	byte_swap_2.S byte_swap_4.S bswap64.c \
	ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcasecmp.c \
	strcpy.S strlen.S strncasecmp.c \
	strncmp.c strncpy.c strrchr.S strtoul.c \
	scanc.S skpc.S \
	random.S
