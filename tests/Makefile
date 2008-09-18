# $NetBSD: Makefile,v 1.8.6.1 2008/09/18 04:37:44 wrstuden Exp $

.include <bsd.own.mk>

SUBDIR=	fs games modules net util

.if ${MKATF} != "no"
SUBDIR+= atf
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
