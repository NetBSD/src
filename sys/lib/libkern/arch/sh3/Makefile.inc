#	$NetBSD: Makefile.inc,v 1.7 2002/08/24 06:39:50 itohy Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap64.c bcmp.c bzero.c ffs.S scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c \
	strncasecmp.c strncmp.c \
	strncpy.c random.c __assert.c memchr.c memcmp.c memset.c \
	bcopy.S byte_swap_2.S byte_swap_4.S memcpy.S memmove.S \
	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstrSI.S \
	mulsi3.S sdivsi3.S udivsi3.S
