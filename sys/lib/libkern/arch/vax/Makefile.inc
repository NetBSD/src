#	$NetBSD: Makefile.inc,v 1.13.4.1 2002/03/16 16:01:55 jdolecek Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c udiv.S urem.S
