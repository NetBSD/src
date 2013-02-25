# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:23 tls Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/misc

FILESDIR=	${KYUA_MISCDIR}
FILESMODE=	444
FILES=		context.html \
		index.html \
		report.css \
		test_result.html
.endif

.include <bsd.files.mk>
