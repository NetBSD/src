# $NetBSD: Makefile,v 1.35 2012/03/17 16:33:10 jruoho Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		bin dev examples fs games include ipf kernel lib libexec net
SUBDIR+=	rump sbin sys toolchain usr.bin usr.sbin

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
