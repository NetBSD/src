#	$NetBSD: Makefile,v 1.4 2009/12/11 21:20:52 pooka Exp $
#

LIB=		ukfs
#LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
#		rumpvfs	${.CURDIR}/../librumpvfs
CPPFLAGS+=	-I${.CURDIR}

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c ukfs_disklabel.c
MAN=		ukfs.3
DBG=		-g

.include <bsd.lib.mk>
