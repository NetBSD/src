#	$NetBSD: Makefile.inc,v 1.18.120.2 2010/03/11 15:04:22 yamt Exp $
#
#	There are likely more that we will notice when we go native

NO_SRCS+= imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
NO_SRCS+= __main.c

SRCS+=	memchr.c memcmp.c random.c scanc.c \
	skpc.c strcat.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	ffs.S memcpy.S memset2.c memmove.S \
	strlen.S strcmp.S
