#	$NetBSD: Makefile.inc,v 1.12 1999/01/15 08:44:27 castor Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	random.c scanc.c skpc.c strcat.c strcpy.c strncmp.c strncpy.c \
	bswap16.c bswap32.c bswap64.c memchr.c memcmp.c memset.c \
	strncasecmp.c __assert.c __main.c
SRCS+=	memcpy.S strlen.S strcmp.S
