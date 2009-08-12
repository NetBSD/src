#	$NetBSD: Makefile.inc,v 1.3 2009/08/12 21:20:40 dsl Exp $
#

SRCS+=	__assert.c __main.c memset.c skpc.c scanc.c random.c
SCRS+=	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
SCRS+=	strcasecmp.c strncasecmp.c strncmp.c strtoul.c ffs.c
SCRS+=	strncpy.c memcmp.c strcat.c strcpy.c strcmp.c strlen.c
SCRS+=	memchr.c

SRCS+=	divdi3.S divsi3.S modsi3.S moddi3.S udivdi3.S udivsi3.S
SCRS+=	umoddi3.S umodsi3.S bswap64.S bswap32.S bswap16.S ntohl.S htonl.S
SCRS+=	memcpy.S
