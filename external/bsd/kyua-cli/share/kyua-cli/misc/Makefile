# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:32 yamt Exp $

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
