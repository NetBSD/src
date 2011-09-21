# $NetBSD: Makefile,v 1.33 2011/09/21 02:14:38 mrg Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		dev fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys toolchain usr.bin usr.sbin util

. if ${MKKMOD} != "no"
SUBDIR+= modules
. endif

. if ${MKCRYPTO} != "no"
SUBDIR+=	crypto
. endif

.include <bsd.test.mk>

.else

.include <bsd.subdir.mk>
.endif
