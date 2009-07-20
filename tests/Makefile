# $NetBSD: Makefile,v 1.16 2009/07/20 17:03:37 joerg Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel lib net rump syscall util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
