#	$NetBSD: Makefile,v 1.2 2020/05/01 22:24:19 christos Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${TESTSBASE}/modules/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
