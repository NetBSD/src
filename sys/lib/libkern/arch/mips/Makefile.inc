#	$NetBSD: Makefile.inc,v 1.17.4.1 2002/01/10 20:00:31 thorpej Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	__assert.c bswap64.c memchr.c memcmp.c memset.c random.c scanc.c \
	skpc.c strcat.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S ffs.S memcpy.S memset.S memmove.S \
	strlen.S strcmp.S
