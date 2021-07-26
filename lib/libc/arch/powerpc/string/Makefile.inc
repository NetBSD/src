#	$NetBSD: Makefile.inc,v 1.15 2021/07/26 12:49:13 rin Exp $

SRCS+=  bzero.S ffs.S strlen.S
NO_SRCS+= memset.S

# XXX
# Disable asm versions that use unaligned memory access and thus break 403.
.if ${MACHINE} == "evbppc"
.  for name in bcopy memcmp memcpy memmove
.    for suffix in o po pico go d
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.else
SRCS+=	memcmp.S bcopy.S memcpy.S memmove.S
.endif
