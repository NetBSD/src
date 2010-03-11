#	$NetBSD: Makefile.inc,v 1.7.68.3 2010/03/11 15:04:22 yamt Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S memcpy.S memmove.S

# XXX: spcopy does not really belong in libkern in the first place
.ifndef RUMPKERNEL
SRCS+=	spcopy.S
.endif

SRCS+= ashrdi3.c divdi3.c    
SRCS+= ffs.c bswap16.c bswap32.c
