# $NetBSD: Makefile,v 1.10 2008/12/08 12:39:04 pooka Exp $

.include <bsd.own.mk>

SUBDIR=	fs games net util

.if ${MACHINE} != "evbppc"
SUBDIR+= modules
.endif

.if ${MKATF} != "no"
SUBDIR+= atf
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
