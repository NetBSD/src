#	$NetBSD: Makefile.inc,v 1.7.68.2 2009/08/19 18:48:19 yamt Exp $

SRCS+=	milli.S
SRCS+=	bcopy.S spcopy.S memcpy.S memmove.S

SRCS+= ashrdi3.c divdi3.c    
SRCS+= ffs.c bswap16.c bswap32.c
