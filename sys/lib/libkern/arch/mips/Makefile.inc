#	$NetBSD: Makefile.inc,v 1.14 1999/02/23 11:06:38 simonb Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	random.c scanc.c skpc.c strcat.c strcpy.c strncmp.c strncpy.c \
	bswap64.c memchr.c memcmp.c memset.c strncasecmp.c __assert.c
SRCS+=	byte_swap_2.S byte_swap_4.S memcpy.S memset.S strlen.S strcmp.S
