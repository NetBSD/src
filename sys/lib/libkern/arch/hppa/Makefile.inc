#	$NetBSD: Makefile.inc,v 1.11.22.1 2014/08/20 00:04:30 tls Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
