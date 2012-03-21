#	$NetBSD: Makefile,v 1.6 2012/03/21 05:38:49 matt Exp $
#

WARNS=		2

LIB=		ukfs
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs
CPPFLAGS+=	-I${.CURDIR}

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c ukfs_disklabel.c
MAN=		ukfs.3

.include <bsd.lib.mk>
