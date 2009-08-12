#	$NetBSD: Makefile.inc,v 1.27 2009/08/12 21:20:40 dsl Exp $

SRCS+=	__main.c __assert.c
SRCS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	memchr.c memcmp.c
SRCS+=	strcat.c strcmp.c strcpy.c strcasecmp.c
SRCS+=	strncasecmp.c strncmp.c strncpy.c
SRCS+=	scanc.c skpc.c
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
SRCS+=	random.c strtoul.c
SRCS+=	syncicache.c

SRCS+=	ffs.S memset.S strlen.S

# Disable the asm versions on evbppc because they break the Explora
.if ${MACHINE} == "evbppc"
memcpy.o: memcpy.c
memcmp.o: memcmp.c
memmove.o: memmove.c
.endif
