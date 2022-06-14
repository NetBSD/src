#	$NetBSD: Makefile.inc,v 1.12.34.1 2022/06/14 10:41:18 martin Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S
SRCS+=	milli_extra.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
