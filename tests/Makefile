# $NetBSD: Makefile,v 1.14 2009/02/20 21:39:57 jmmv Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel net util syscall

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
