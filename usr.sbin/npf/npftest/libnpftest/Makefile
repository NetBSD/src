# $NetBSD: Makefile,v 1.14 2025/07/20 12:25:54 joe Exp $
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
SRCS+=		npf_gc_test.c
SRCS+=		npf_state_test.c
SRCS+=		npf_rule_test.c
SRCS+=		npf_nat_test.c
SRCS+=		npf_rid_test.c
SRCS+=		npf_l2rule_test.c
SRCS+=		npfl2onlytest.c

SRCS+=		npf_perf_test.c

CPPFLAGS+=	-D_NPF_TESTING
CPPFLAGS+=	-I${.CURDIR}/../../../../sys/net/npf
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

WARNS=		5

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
