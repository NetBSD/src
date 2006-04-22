#	$NetBSD: Makefile.inc,v 1.6.6.1 2006/04/22 11:40:05 simonb Exp $

#	$OpenBSD: Makefile.inc,v 1.14 2000/12/29 19:04:41 mickey Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	random.c ashrdi3.c divdi3.c strncpy.c strncmp.c strcmp.c memchr.c \
	memcmp.c memset.c strlen.c strcpy.c \
	strcat.c skpc.c strncasecmp.c strtoul.c \
	bcmp.c scanc.c ffs.c strlcpy.c bswap16.c bswap32.c bswap64.c bzero.c
SRCS+=	__assert.c

SRCS+=	milli.S

SRCS+=	bcopy.S spcopy.S memcpy.S memmove.S
