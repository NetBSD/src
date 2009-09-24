# $NetBSD: Makefile,v 1.17 2009/09/24 21:33:45 pooka Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel lib libexec net rump syscall util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
