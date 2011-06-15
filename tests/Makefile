# $NetBSD: Makefile,v 1.31 2011/06/15 23:45:45 mrg Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		crypto dev fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys syscall toolchain usr.bin usr.sbin util

. if ${MKKMOD} != "no"
SUBDIR+= modules
. endif

.include <bsd.test.mk>

.else

.include <bsd.subdir.mk>
.endif
