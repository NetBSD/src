# $NetBSD: Makefile,v 1.3.6.1 2014/05/22 15:40:14 yamt Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/tools
.PATH:		${SRCDIR}/tools/sample

FILESDIR=	/usr/share/examples/atf
FILESMODE=	444
FILES=		atf-run.hooks \
		tests-results.css
.endif

.include <bsd.prog.mk>
