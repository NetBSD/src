#	$NetBSD: Makefile.inc,v 1.4 2000/06/09 04:42:46 msaitoh Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap64.c bcmp.c bzero.c ffs.c scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c \
	strncpy.c random.c __assert.c memchr.c memcmp.c memset.c \
	bcopy.S memcpy.S memmove.S \
	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstrSI.S \
	mulsi3.S sdivsi3.S udivsi3.S
