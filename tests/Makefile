# $NetBSD: Makefile,v 1.9 2008/08/16 23:51:51 gmcgarry Exp $

.include <bsd.own.mk>

SUBDIR=	fs games modules net util

.if ${MKATF} != "no"
SUBDIR+= atf
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
