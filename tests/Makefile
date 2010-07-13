# $NetBSD: Makefile,v 1.21 2010/07/13 21:13:21 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= fs net kernel lib libexec rump sys syscall

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

.include <bsd.test.mk>
.include <bsd.subdir.mk>
