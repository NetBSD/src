#	$NetBSD: Makefile.inc,v 1.16 2002/09/19 17:37:32 ragge Exp $
#
#	The rest of the lib routines are in machine/macros.h
#
SRCS+=	__assert.c __main.c blkcpy.S blkset.S bswap64.S random.S \
	strcasecmp.c strncasecmp.c strncmp.c strtoul.c udiv.S urem.S
