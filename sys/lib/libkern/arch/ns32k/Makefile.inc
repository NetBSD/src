#	$NetBSD: Makefile.inc,v 1.20 2002/09/21 17:45:17 chs Exp $

SRCS+=	__main.c __assert.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	byte_swap_2.S byte_swap_4.S bswap64.c \
	bcmp.S bcopy.S bzero.S ffs.S \
	memchr.S memcmp.S memcpy.S memmove.S memset.S \
	strcat.S strchr.S strcmp.S strcpy.S strlen.S strrchr.S strcasecmp.c \
	strncasecmp.c strncmp.S strncpy.S strtoul.c \
	scanc.c skpc.c \
	random.S
