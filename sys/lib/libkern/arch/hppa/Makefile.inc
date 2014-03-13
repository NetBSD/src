#	$NetBSD: Makefile.inc,v 1.12 2014/03/13 16:09:11 pooka Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
