#	$NetBSD: Makefile.inc,v 1.36 2026/05/05 21:52:42 mrg Exp $

SRCS+=	bswap16.c bswap32.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	syncicache.c

SRCS+=	ffs.S memset.S strlen.S
SRCS+=	gprsavrest.S

.if !empty(LIBKERN_MD_FLAGS:M*strict-align*)
# Disable asm versions that use unaligned memory access and thus break 403.
.  for name in memcmp memcpy memmove
.    for suffix in o po pico go d
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.endif
