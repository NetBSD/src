#	$NetBSD: Makefile,v 1.1.4.2 2019/06/10 22:10:08 christos Exp $

.include <bsd.own.mk>

KMOD=		threadpool_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		threadpool_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
