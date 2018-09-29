# $NetBSD: Makefile,v 1.9 2018/09/29 14:41:36 rmind Exp $
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

CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

WARNS=		5

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
