#	$NetBSD: Makefile,v 1.5.6.2 2014/05/22 11:37:00 yamt Exp $
#

WARNS=		2

LIB=		ukfs
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs \
		pthread	${.CURDIR}/../libpthread
CPPFLAGS+=	-I${.CURDIR}

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c ukfs_disklabel.c
MAN=		ukfs.3

.include <bsd.lib.mk>
