#	$NetBSD: Makefile.inc,v 1.15 2002/02/24 00:08:22 matt Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c udiv.S urem.S
