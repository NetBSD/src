# $NetBSD: Makefile,v 1.11 2009/01/19 07:08:14 jmmv Exp $

.include <bsd.own.mk>

SUBDIR=	fs games net util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
