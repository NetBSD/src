#	$NetBSD: Makefile.inc,v 1.14 2002/02/10 22:04:51 thorpej Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__main.c __assert.c blkcpy.S blkset.S random.S strncmp.c \
	strcasecmp.c strncasecmp.c bswap64.S
