#	$NetBSD: Makefile,v 1.4 2016/01/23 21:22:46 christos Exp $
#

LIB=		p2k
LIBDPLIBS+=	puffs	${.CURDIR}/../libpuffs		\
		rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs	\
		ukfs	${.CURDIR}/../libukfs

INCS=		p2k.h
INCSDIR=	/usr/include/rump
CPPFLAGS+=	-D_KERNTYPES

SRCS=		p2k.c
MAN=		p2k.3

.include <bsd.lib.mk>
