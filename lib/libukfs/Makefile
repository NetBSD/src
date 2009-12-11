#	$NetBSD: Makefile,v 1.5 2009/12/11 21:21:52 pooka Exp $
#

LIB=		ukfs
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs
CPPFLAGS+=	-I${.CURDIR}

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c ukfs_disklabel.c
MAN=		ukfs.3

.include <bsd.lib.mk>
