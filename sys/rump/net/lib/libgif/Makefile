#	$NetBSD: Makefile,v 1.1.2.2 2015/12/27 12:10:17 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netinet	\
	${.CURDIR}/../../../../netinet6

LIB=	rumpnet_gif
COMMENT= generic tunnel interface

IOCONF=	GIF.ioconf
SRCS=	if_gif.c in_gif.c in6_gif.c ip_ecn.c

SRCS+=	gif_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
