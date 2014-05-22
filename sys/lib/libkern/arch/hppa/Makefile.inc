#	$NetBSD: Makefile.inc,v 1.11.12.1 2014/05/22 11:41:04 yamt Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ffs.c bswap16.c bswap32.c
