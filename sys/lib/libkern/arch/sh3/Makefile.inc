#	$NetBSD: Makefile.inc,v 1.8 2002/09/05 08:35:16 msaitoh Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap64.c bcmp.c bzero.c ffs.S scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c \
	strncasecmp.c strncmp.c \
	strncpy.c random.c __assert.c memchr.c memcmp.c memset.c \
	bcopy.S byte_swap_2.S byte_swap_4.S memcpy.S memmove.S \
	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstr_i4.S \
	movstrSI.S mulsi3.S sdivsi3.S udivsi3.S
