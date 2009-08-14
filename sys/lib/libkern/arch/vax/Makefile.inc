#	$NetBSD: Makefile.inc,v 1.23 2009/08/14 19:23:55 dsl Exp $
#
#	The rest of the lib routines are in machine/macros.h
#

SRCS+=	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S
SRCS+=	random.S
SRCS+=	udiv.S urem.S

NO_SRCS+= imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
NO_SRCS+= memset.c
NO_SRCS+= scanc.c skpc.c
