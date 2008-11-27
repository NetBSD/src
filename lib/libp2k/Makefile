#	$NetBSD: Makefile,v 1.2 2008/11/27 16:14:46 pooka Exp $
#

LIB=		p2k
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs	\
		ukfs	${.CURDIR}/../libukfs

INCS=		p2k.h
INCSDIR=	/usr/include/rump

SRCS=		p2k.c
MAN=		p2k.3

.include <bsd.lib.mk>
