# $NetBSD: Makefile,v 1.22 2010/07/17 19:26:27 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=	crypto games ipf util
  
.if ${MKATF} != "no"
SUBDIR+= fs include kernel lib libexec net rump sys syscall

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.endif

.include <bsd.test.mk>
.include <bsd.subdir.mk>
