# $NetBSD: Makefile,v 1.3.6.1 2014/05/22 15:40:15 yamt Exp $

.include <bsd.init.mk>

SUBDIR=		atf-sh
.if !(${MKKYUA} != "no")
SUBDIR+=	atf-config atf-report atf-run atf-version
.endif

.include <bsd.subdir.mk>
