#	$NetBSD: Makefile,v 1.1 2020/04/26 09:08:41 maxv Exp $

.include <bsd.own.mk>

KMOD=		x86_pte_tester
KMODULEDIR=	${DESTDIR}/${TESTSBASE}/modules/${KMOD}

SRCS=		x86_pte_tester.c

ATFFILE=	no
NOMAN=		# defined

.include <bsd.test.mk>
.include <bsd.kmodule.mk>
