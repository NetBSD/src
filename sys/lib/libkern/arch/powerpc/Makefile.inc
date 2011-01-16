#	$NetBSD: Makefile.inc,v 1.29 2011/01/16 03:22:19 matt Exp $

SRCS+=	bswap16.c bswap32.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	syncicache.c

SRCS+=	ffs.S memset.S strlen.S

CPPFLAGS+=	-D_NOREGNAMES

# Disable the asm versions on evbppc because they break the Explora
.if ${MACHINE} == "evbppc"
memcpy.o: memcpy.c
memcmp.o: memcmp.c
memmove.o: memmove.c
.endif
