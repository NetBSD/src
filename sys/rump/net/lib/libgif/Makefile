#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:17 jdolecek Exp $
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
