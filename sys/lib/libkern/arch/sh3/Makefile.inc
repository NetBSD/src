#	$NetBSD: Makefile.inc,v 1.6.2.4 2002/12/11 06:46:25 thorpej Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap64.c bcmp.c bzero.S ffs.S scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strcasecmp.c \
	strncasecmp.c strncmp.c strtoul.c \
	strncpy.c random.c __assert.c memchr.c memcmp.c memset.S \
	bcopy.S byte_swap_2.S byte_swap_4.S memcpy.S memmove.S \
	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstr_i4.S \
	movstrSI.S mulsi3.S sdivsi3.S udivsi3.S
