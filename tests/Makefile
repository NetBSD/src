# $NetBSD: Makefile,v 1.33.2.2 2012/10/30 18:59:50 yamt Exp $

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

. if ${MKSHARE} != "no"
SUBDIR+=	share
. endif

.include <bsd.test.mk>

.else

.include <bsd.subdir.mk>
.endif
