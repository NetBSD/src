# $NetBSD: Makefile,v 1.38 2012/08/08 13:57:05 christos Exp $

.include <bsd.own.mk>

.if ${MKATF} != "no"

TESTSDIR=	${TESTSBASE}
ATFFILE=	yes

SUBDIR=		bin dev examples games include kernel lib libexec net
SUBDIR+=	sbin sys usr.bin usr.sbin

. if (${MKRUMP} != "no")
SUBDIR+= fs rump

. if ${MKKMOD} != "no"
SUBDIR+= modules
. endif
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
