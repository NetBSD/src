#	$NetBSD: Makefile.inc,v 1.11 1998/09/11 16:46:34 jonathan Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	random.c scanc.c skpc.c strcat.c strcpy.c strncmp.c strncpy.c \
	bswap16.c bswap32.c bswap64.c memchr.c memcmp.c memset.c \
	strncasecmp.c __assert.c
SRCS+=	memcpy.S
