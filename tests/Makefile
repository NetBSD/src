# $NetBSD: Makefile,v 1.9.8.1 2009/08/26 04:02:14 matt Exp $

.include <bsd.own.mk>

SUBDIR=	fs games modules util

.if ${MKATF} != "no"
SUBDIR+= atf net 
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
