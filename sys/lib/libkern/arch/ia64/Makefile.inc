#	$NetBSD: Makefile.inc,v 1.4.44.1 2016/08/06 00:19:09 pgoyette Exp $
#

SRCS+=	ffs.c

SRCS+=	divdi3.S divsi3.S modsi3.S moddi3.S udivdi3.S udivsi3.S
SRCS+=	umoddi3.S umodsi3.S bswap64.S bswap32.S bswap16.S ntohl.S htonl.S
SRCS+=	bcopy.S bzero.S ffs.S memcpy.S memmove.S
