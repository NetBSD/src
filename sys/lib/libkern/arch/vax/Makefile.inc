#	$NetBSD: Makefile.inc,v 1.11.8.2 2000/11/22 16:05:39 bouyer Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__main.c __assert.c random.S strncmp.c strcasecmp.c \
	strncasecmp.c bswap64.S
