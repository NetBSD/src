#	$NetBSD: Makefile.inc,v 1.23 1999/01/15 13:31:30 bouyer Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	byte_swap_2.S byte_swap_4.S bswap64.c \
	bcmp.S bzero.S ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strncasecmp.c \
	strncmp.c strncpy.c strrchr.S \
	scanc.S skpc.S \
	random.S
