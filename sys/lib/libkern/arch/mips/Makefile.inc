#	$NetBSD: Makefile.inc,v 1.19 2009/08/12 21:20:40 dsl Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	__assert.c bswap64.c memchr.c memcmp.c memset.c random.c scanc.c
SRCS+=	skpc.c strcat.c strcpy.c strcasecmp.c
SRCS+=	strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S memcpy.S memset.S memmove.S
SRCS+=	strlen.S strcmp.S
