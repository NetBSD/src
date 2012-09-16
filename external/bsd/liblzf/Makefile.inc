# $NetBSD: Makefile.inc,v 1.2 2012/09/16 18:59:27 tls Exp $
.include <bsd.own.mk>

USE_FORT?=	yes	# data-driven bugs?

LIBLZFDIR=	${NETBSDSRCDIR}/external/bsd/liblzf
.PATH:          ${LIBLZFDIR}/dist
CPPFLAGS+=	-I ${LIBLZFDIR}/dist
