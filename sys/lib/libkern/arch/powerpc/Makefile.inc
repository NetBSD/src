#	$NetBSD: Makefile.inc,v 1.31.70.1 2021/08/01 22:42:39 thorpej Exp $

SRCS+=	bswap16.c bswap32.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	syncicache.c

SRCS+=	ffs.S memset.S strlen.S
SRCS+=	gprsavrest.S

# XXX
# Disable asm versions that use unaligned memory access and thus break 403.
.if ${MACHINE} == "evbppc"
.  for name in memcmp memcpy memmove
.    for suffix in o po pico go d
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.endif
