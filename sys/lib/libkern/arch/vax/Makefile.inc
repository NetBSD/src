#	$NetBSD: Makefile.inc,v 1.13 2000/11/01 19:37:20 thorpej Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__main.c __assert.c random.S strncmp.c strcasecmp.c \
	strncasecmp.c bswap64.S
