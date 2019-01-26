#	$NetBSD: Makefile,v 1.1.2.2 2019/01/26 22:00:38 pgoyette Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
