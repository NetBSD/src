# $NetBSD: Makefile,v 1.20 2010/03/15 19:03:08 jmmv Exp $

.include <bsd.own.mk>

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= fs net kernel lib libexec rump sys syscall

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
