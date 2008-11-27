#	$NetBSD: Makefile,v 1.2.2.2 2008/11/27 16:14:47 pooka Exp $
#

LIB=		ukfs
LIBDPLIBS+=	rump	${.CURDIR}/../librump		\
		rumpvfs	${.CURDIR}/../librumpvfs

INCS=		ukfs.h
INCSDIR=	/usr/include/rump

SRCS=		ukfs.c
MAN=		ukfs.3

.include <bsd.lib.mk>
