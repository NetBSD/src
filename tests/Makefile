# $NetBSD: Makefile,v 1.28.2.2 2011/03/05 15:10:53 bouyer Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		crypto dev fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys syscall toolchain usr.bin usr.sbin util

. if ${MACHINE} != "evbppc" && ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.include <bsd.test.mk>

.else

.include <bsd.subdir.mk>
.endif
