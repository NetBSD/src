#	$NetBSD: Makefile.inc,v 1.18.120.1 2009/08/19 18:48:19 yamt Exp $
#
#	There are likely more that we will notice when we go native

NO_SRCS+= imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
NO_SRCS+= __main.c

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S memcpy.S memset.S memmove.S
SRCS+=	strlen.S strcmp.S
