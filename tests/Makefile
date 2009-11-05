# $NetBSD: Makefile,v 1.18 2009/11/05 17:34:25 dyoung Exp $

.include <bsd.own.mk>

SUBDIR=	crypto fs games ipf kernel lib libexec net rump syscall util

.if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
