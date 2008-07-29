#	$NetBSD: Makefile,v 1.1 2008/07/29 13:17:40 pooka Exp $
#

LIB=		p2k
LIBDPLIBS+=     rump    ${.CURDIR}/../librump	\
		ukfs	${.CURDIR}/../libukfs

INCS=		p2k.h
INCSDIR=	/usr/include/rump

SRCS=		p2k.c
MAN=		p2k.3

.include <bsd.lib.mk>
