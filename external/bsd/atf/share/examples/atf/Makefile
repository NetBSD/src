# $NetBSD: Makefile,v 1.4 2014/02/08 19:15:34 jmmv Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools
.PATH:		${SRCDIR}/tools/sample

FILESDIR=	/usr/share/examples/atf
FILESMODE=	444
FILES=		atf-run.hooks \
		tests-results.css
.endif

.include <bsd.prog.mk>
