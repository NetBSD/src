#	$NetBSD: Makefile.inc,v 1.15.8.1 2000/11/20 18:09:29 bouyer Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	__assert.c bswap64.c memchr.c memcmp.c memset.c random.c scanc.c \
	skpc.c strcat.c strcpy.c strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S memcpy.S memset.S memmove.S \
	strlen.S strcmp.S
