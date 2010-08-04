# $NetBSD: Makefile,v 1.23 2010/08/04 13:15:15 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= dev fs include kernel lib libexec net rump sys syscall

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

.include <bsd.test.mk>
.include <bsd.subdir.mk>
