#	$NetBSD: Makefile.inc,v 1.2 1999/10/21 15:05:08 msaitoh Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	bswap64.c bcmp.c bzero.c ffs.c scanc.c skpc.c \
	strcat.c strcmp.c strcpy.c strlen.c strncasecmp.c strncmp.c \
	strncpy.c random.c __assert.c memchr.c memcmp.c memset.c \
	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S \
	mulsi3.S sdivsi3.S udivsi3.S
