#	$NetBSD: Makefile.inc,v 1.13 1999/01/15 13:31:31 bouyer Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	random.c scanc.c skpc.c strcat.c strcpy.c strncmp.c strncpy.c \
	byte_swap_2.S byte_swap_4.S bswap64.c memchr.c memcmp.c memset.c \
	strncasecmp.c __assert.c __main.c
SRCS+=	memcpy.S strlen.S strcmp.S
