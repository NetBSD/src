#	$NetBSD: Makefile.inc,v 1.33 2021/07/26 12:49:13 rin Exp $

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
