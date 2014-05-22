#	$NetBSD: Makefile,v 1.2.8.1 2014/05/22 11:36:58 yamt Exp $
#

LIB=		p2k
LIBDPLIBS+=	puffs	${.CURDIR}/../libpuffs		\
		rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs	\
		ukfs	${.CURDIR}/../libukfs

INCS=		p2k.h
INCSDIR=	/usr/include/rump

SRCS=		p2k.c
MAN=		p2k.3

.include <bsd.lib.mk>
