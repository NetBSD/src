#	$NetBSD: Makefile.inc,v 1.16 2022/05/30 14:43:37 rin Exp $

SRCS+=  bzero.S ffs.S strlen.S
NO_SRCS+= memset.S

# with unaligned memory access
SRCS+=	memcmp.S bcopy.S memcpy.S memmove.S
