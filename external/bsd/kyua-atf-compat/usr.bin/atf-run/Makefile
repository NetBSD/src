# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:22 yamt Exp $

.include <bsd.init.mk>

FILES=		atf-run
FILESDIR=	/usr/bin
FILESMODE=	${BINMODE}

realall: atf-run
atf-run: atf-run.sh
	src=atf-run.sh dst=atf-run; ${BUILD_SCRIPT}
CLEANFILES+=	atf-run atf-run.new

MAN=		atf-run.1

.include <bsd.prog.mk>
.include <bsd.files.mk>
