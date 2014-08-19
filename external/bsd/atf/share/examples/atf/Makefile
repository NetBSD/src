# $NetBSD: Makefile,v 1.3.12.1 2014/08/19 23:45:55 tls Exp $

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
