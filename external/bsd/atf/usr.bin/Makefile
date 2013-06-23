# $NetBSD: Makefile,v 1.3.12.1 2013/06/23 06:26:23 tls Exp $

.include <bsd.init.mk>

SUBDIR=		atf-sh
.if !(${MKKYUA} != "no")
SUBDIR+=	atf-config atf-report atf-run atf-version
.endif

.include <bsd.subdir.mk>
