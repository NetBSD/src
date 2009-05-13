# $NetBSD: Makefile,v 1.11.2.1 2009/05/13 19:19:18 jym Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel net rump syscall util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
