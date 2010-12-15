# $NetBSD: Makefile,v 1.26 2010/12/15 10:36:50 haad Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		crypto dev fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys syscall util usr.sbin

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.include <bsd.test.mk>
.endif

.include <bsd.subdir.mk>
