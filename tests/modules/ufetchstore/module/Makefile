#	$NetBSD: Makefile,v 1.1 2019/04/06 03:06:29 thorpej Exp $

.include <bsd.own.mk>

KMOD=		ufetchstore_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		ufetchstore_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
