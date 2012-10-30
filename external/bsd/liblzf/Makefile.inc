# $NetBSD: Makefile.inc,v 1.1.6.1 2012/10/30 18:55:45 yamt Exp $
.include <bsd.own.mk>

USE_FORT?=	yes	# data-driven bugs?

LIBLZFDIR=	${NETBSDSRCDIR}/external/bsd/liblzf
.PATH:          ${LIBLZFDIR}/dist
CPPFLAGS+=	-I ${LIBLZFDIR}/dist
