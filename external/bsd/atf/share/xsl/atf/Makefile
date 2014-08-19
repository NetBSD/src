# $NetBSD: Makefile,v 1.3.12.1 2014/08/19 23:45:55 tls Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/tools

FILESDIR=	/usr/share/xsl/atf
FILESMODE=	444
FILES=		tests-results.xsl
.endif

.include <bsd.prog.mk>
