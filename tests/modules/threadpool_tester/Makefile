#	$NetBSD: Makefile,v 1.1 2019/01/25 18:33:59 christos Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
