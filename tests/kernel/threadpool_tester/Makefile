#	$NetBSD: Makefile,v 1.1.2.2 2018/12/26 14:02:09 pgoyette Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/kernel/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
