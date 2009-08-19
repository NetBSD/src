#	$NetBSD: Makefile.inc,v 1.1.74.1 2009/08/19 18:48:19 yamt Exp $
#

SRCS+=	ffs.c

SRCS+=	divdi3.S divsi3.S modsi3.S moddi3.S udivdi3.S udivsi3.S
SRCS+=	umoddi3.S umodsi3.S bswap64.S bswap32.S bswap16.S ntohl.S htonl.S
SRCS+=	memcpy.S
