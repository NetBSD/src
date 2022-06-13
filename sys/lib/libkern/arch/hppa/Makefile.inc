#	$NetBSD: Makefile.inc,v 1.13 2022/06/13 16:03:07 skrll Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S
SRCS+=	milli_extra.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
