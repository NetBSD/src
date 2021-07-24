#	$NetBSD: Makefile.inc,v 1.32 2021/07/24 05:29:26 rin Exp $

SRCS+=	bswap16.c bswap32.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	syncicache.c

SRCS+=	ffs.S memset.S strlen.S
SRCS+=	gprsavrest.S

# Disable the asm versions on evbppc because they break the Explora
.if ${MACHINE} == "evbppc"
.  for name in memcmp memcpy memmove
.    for suffix in o po pico go
${name}.${suffix}: ${name}.c
.    endfor
.  endfor
.endif
