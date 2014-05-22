# $NetBSD: Makefile,v 1.3.6.1 2014/05/22 15:40:14 yamt Exp $

.include <bsd.init.mk>

.if ${MKSHARE} != "no"
.PATH:		${SRCDIR}/tools

FILESDIR=	/usr/share/xml/atf
FILESMODE=	444
FILES=		tests-results.dtd
.endif

.include <bsd.prog.mk>
