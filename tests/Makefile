# $NetBSD: Makefile,v 1.15 2009/05/02 16:02:18 pooka Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel net rump syscall util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
