#	$NetBSD: Makefile,v 1.3 2019/04/15 23:41:23 christos Exp $

.include <bsd.own.mk>

KMOD=		ufetchstore_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		ufetchstore_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
