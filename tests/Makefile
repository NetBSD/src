# $NetBSD: Makefile,v 1.33.2.1 2012/04/17 00:09:01 yamt Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		bin dev examples fs games include kernel lib libexec net
SUBDIR+=	rump sbin sys usr.bin usr.sbin

. if ${MKKMOD} != "no"
SUBDIR+= modules
. endif

. if ${MKCRYPTO} != "no"
SUBDIR+=	crypto
. endif

. if ${MKIPFILTER} != "no"
SUBDIR+=	ipf
. endif

.include <bsd.test.mk>

.else

.include <bsd.subdir.mk>
.endif
