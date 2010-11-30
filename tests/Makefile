# $NetBSD: Makefile,v 1.24 2010/11/30 23:17:27 riz Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= dev fs include kernel lib libexec net rump sbin sys syscall

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

.include <bsd.test.mk>
.include <bsd.subdir.mk>
