#	$NetBSD: Makefile.inc,v 1.1 2022/05/30 15:06:55 rin Exp $

# Disable asm versions that use unaligned memory access.

.PATH:	${NETBSDSRCDIR}/common/lib/libc/string

SRCS+=	memcmp.c bcopy.c memcpy.c memmove.c

.if 0
.  for name in bcopy memcmp memcpy memmove
.    for suffix in o po pico go d
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.endif
