#	$NetBSD: Makefile.inc,v 1.10 2009/08/14 19:23:53 dsl Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S spcopy.S memcpy.S memmove.S

SRCS+= ashrdi3.c divdi3.c    
SRCS+= ffs.c bswap16.c bswap32.c
