#	$NetBSD: Makefile.inc,v 1.2 2022/05/30 15:18:32 rin Exp $

# Disable asm versions that use unaligned memory access.

.PATH:	${NETBSDSRCDIR}/common/lib/libc/string

SRCS+=	memcmp.c bcopy.c memcpy.c memmove.c
