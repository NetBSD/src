# $NetBSD: Makefile,v 1.9.2.3 2013/02/11 21:49:48 riz Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=		npf_inet.c npf_instr.c npf_mbuf.c npf_nat.c
SRCS+=		npf_processor.c npf_ruleset.c npf_rproc.c npf_sendpkt.c
SRCS+=		npf_session.c npf_state.c npf_state_tcp.c
SRCS+=		npf_tableset.c npf_tableset_ptree.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
