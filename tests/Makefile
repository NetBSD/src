# $NetBSD: Makefile,v 1.25 2010/12/08 23:56:02 njoly Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		crypto dev fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys syscall util

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.include <bsd.test.mk>
.endif

.include <bsd.subdir.mk>
