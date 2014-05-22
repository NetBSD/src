# $NetBSD: Makefile,v 1.1.2.4 2014/05/22 11:43:07 yamt Exp $
#
# Public Domain
#

RUMPTOP=	${.CURDIR}/../../../../sys/rump

LIB=		npftest
LIBISPRIVATE=	yes

SRCS+=		npf_test_subr.c
SRCS+=		npf_mbuf_subr.c

SRCS+=		npf_nbuf_test.c
SRCS+=		npf_bpf_test.c
SRCS+=		npf_table_test.c
SRCS+=		npf_state_test.c
SRCS+=		npf_rule_test.c
SRCS+=		npf_nat_test.c

SRCS+=		npf_perf_test.c

CPPFLAGS+=	-D_NPF_TESTING
CPPFLAGS+=	-I${.CURDIR}/../../../../sys/net/npf
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

WARNS=		5

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
