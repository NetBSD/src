#	$NetBSD: Makefile.inc,v 1.15 1999/03/15 07:04:04 jonathan Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	random.c scanc.c skpc.c strcat.c strcpy.c strncmp.c strncpy.c \
	bswap64.c memchr.c memcmp.c memset.c strncasecmp.c __assert.c
SRCS+=	byte_swap_2.S byte_swap_4.S memcpy.S memset.S memmove.S \
	strlen.S strcmp.S
