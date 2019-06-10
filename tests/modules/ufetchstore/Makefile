#	$NetBSD: Makefile,v 1.3.2.2 2019/06/10 22:10:08 christos Exp $

.include <bsd.own.mk>

KMOD=		ufetchstore_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		ufetchstore_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
