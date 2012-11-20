# $NetBSD: Makefile.inc,v 1.1.12.1 2012/11/20 02:57:59 tls Exp $
.include <bsd.own.mk>

USE_FORT?=	yes	# data-driven bugs?

LIBLZFDIR=	${NETBSDSRCDIR}/external/bsd/liblzf
.PATH:          ${LIBLZFDIR}/dist
CPPFLAGS+=	-I ${LIBLZFDIR}/dist
