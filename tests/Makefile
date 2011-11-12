# $NetBSD: Makefile,v 1.34 2011/11/12 01:19:25 jmmv Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		dev examples fs games include ipf kernel lib libexec net
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
