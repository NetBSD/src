#	$NetBSD: Makefile.inc,v 1.1.10.2 2006/05/24 15:50:42 tron Exp $
#

SRCS+=	__assert.c __main.c memset.c skpc.c scanc.c random.c \
	imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c ffs.c \
	strncpy.c memcmp.c strcat.c strcpy.c strcmp.c strlen.c \
	memchr.c

SRCS+=	divdi3.S divsi3.S modsi3.S moddi3.S udivdi3.S udivsi3.S \
	umoddi3.S umodsi3.S bswap32.S bswap16.S ntohl.S htonl.S \
	memcpy.S
