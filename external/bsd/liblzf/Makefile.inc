# $NetBSD: Makefile.inc,v 1.1 2010/02/03 06:25:55 tls Exp $
.include <bsd.own.mk>

LIBLZFDIR=	${NETBSDSRCDIR}/external/bsd/liblzf
.PATH:          ${LIBLZFDIR}/dist
CPPFLAGS+=	-I ${LIBLZFDIR}/dist
