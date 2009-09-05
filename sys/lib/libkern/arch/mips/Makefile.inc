#	$NetBSD: Makefile.inc,v 1.18.138.2 2009/09/05 18:46:38 matt Exp $
#
#	There are likely more that we will notice when we go native

SRCS+=	__assert.c memchr.c memcmp.c random.c scanc.c \
	skpc.c strcat.c strcpy.c strcasecmp.c \
	strncasecmp.c strncmp.c strncpy.c strtoul.c

SRCS+=	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	ffs.S memcpy.S memset2.c memmove.S \
	strlen.S strcmp.S
