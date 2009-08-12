#	$NetBSD: Makefile.inc,v 1.9 2009/08/12 21:20:40 dsl Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S spcopy.S memcpy.S memmove.S

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	random.c ashrdi3.c divdi3.c strncpy.c strncmp.c strcmp.c memchr.c
SRCS+=	memcmp.c memset.c strlen.c strcpy.c
SRCS+=	strcat.c skpc.c strncasecmp.c strtoul.c
SRCS+=	scanc.c ffs.c strlcpy.c bswap16.c bswap32.c bswap64.c
SRCS+=	__assert.c
