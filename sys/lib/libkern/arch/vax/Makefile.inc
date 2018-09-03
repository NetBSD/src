#	$NetBSD: Makefile.inc,v 1.25 2018/09/03 16:54:55 riastradh Exp $
#
#	The rest of the lib routines are in machine/macros.h
#

SRCS+=	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S
SRCS+=	random.S
SRCS+=	udiv.S urem.S

NO_SRCS+= imax.c imin.c lmax.c lmin.c uimax.c uimin.c ulmax.c ulmin.c
NO_SRCS+= scanc.c skpc.c
