# $NetBSD: Makefile,v 1.19 2009/12/15 03:01:48 mrg Exp $

.include <bsd.own.mk>

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= fs net kernel lib libexec rump syscall 

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

TESTSDIR=	${TESTSBASE}

.include <bsd.test.mk>
.include <bsd.subdir.mk>
