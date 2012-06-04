# $NetBSD: Makefile,v 1.3 2012/06/04 00:28:34 rmind Exp $
#
# Public Domain
#

RUMPTOP=	${.CURDIR}/../../../../sys/rump

LIB=		npftest
LIBISPRIVATE=	yes

SRCS+=		npf_test_subr.c
SRCS+=		npf_mbuf_subr.c

SRCS+=		npf_nbuf_test.c
SRCS+=		npf_processor_test.c
SRCS+=		npf_table_test.c
SRCS+=		npf_state_test.c

CPPFLAGS+=	-D_NPF_TESTING
CPPFLAGS+=	-I${.CURDIR}/../../../../sys/net/npf
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

WARNS=		4

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
