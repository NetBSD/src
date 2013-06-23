# $NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:26:39 tls Exp $

.include <bsd.init.mk>

FILES=		atf-report
FILESDIR=	/usr/bin
FILESMODE=	${BINMODE}

realall: atf-report
atf-report: atf-report.sh
	src=atf-report.sh dst=atf-report; ${BUILD_SCRIPT}
CLEANFILES+=	atf-report atf-report.new

MAN=		atf-report.1

.include <bsd.prog.mk>
.include <bsd.files.mk>
