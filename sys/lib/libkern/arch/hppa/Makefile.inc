#	$NetBSD: Makefile.inc,v 1.11.26.1 2014/05/18 17:46:08 rmind Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
