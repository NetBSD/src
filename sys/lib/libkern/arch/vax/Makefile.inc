#	$NetBSD: Makefile.inc,v 1.23.2.1 2010/04/30 14:44:15 uebayasi Exp $
#
#	The rest of the lib routines are in machine/macros.h
#

SRCS+=	blkcpy.S blkset.S bswap16.S bswap32.S bswap64.S
SRCS+=	random.S
SRCS+=	udiv.S urem.S

NO_SRCS+= imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c
NO_SRCS+= scanc.c skpc.c
