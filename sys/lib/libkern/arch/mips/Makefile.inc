#	$NetBSD: Makefile.inc,v 1.18.138.1 2009/08/18 15:27:42 matt Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	__assert.c bswap64.c memchr.c memcmp.c random.c scanc.c \
	skpc.c strcat.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S memcpy.S memset2.c memmove.S \
	strlen.S strcmp.S
