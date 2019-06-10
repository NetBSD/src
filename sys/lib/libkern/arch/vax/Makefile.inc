#	$NetBSD: Makefile.inc,v 1.24.62.1 2019/06/10 22:09:04 christos Exp $
#
#	The rest of the lib routines are in machine/macros.h
#

SRCS+=	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S
SRCS+=	random.S
SRCS+=	udiv.S urem.S

NO_SRCS+= imax.c imin.c lmax.c lmin.c uimax.c uimin.c ulmax.c ulmin.c
NO_SRCS+= scanc.c skpc.c
