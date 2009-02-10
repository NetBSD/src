# $NetBSD: Makefile,v 1.12 2009/02/10 13:43:54 pooka Exp $

.include <bsd.own.mk>

SUBDIR=	fs games net util syscall

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
