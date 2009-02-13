# $NetBSD: Makefile,v 1.13 2009/02/13 20:58:13 jmmv Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games net util syscall

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
