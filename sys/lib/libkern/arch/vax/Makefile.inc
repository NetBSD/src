#	$NetBSD: Makefile.inc,v 1.13.4.2 2002/10/10 18:43:27 jdolecek Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c udiv.S urem.S
