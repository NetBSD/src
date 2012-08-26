# $NetBSD: Makefile,v 1.39 2012/08/26 23:03:20 jmmv Exp $

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
