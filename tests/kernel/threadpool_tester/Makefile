#	$NetBSD: Makefile,v 1.1 2018/12/24 16:58:54 thorpej Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/kernel/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
