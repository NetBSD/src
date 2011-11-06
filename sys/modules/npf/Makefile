# $NetBSD: Makefile,v 1.5 2011/11/06 10:35:50 tron Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_alg.c npf_alg_icmp.c npf_ctl.c npf_handler.c
SRCS+=		npf_inet.c npf_instr.c npf_log.c npf_mbuf.c npf_nat.c
SRCS+=		npf_processor.c npf_ruleset.c npf_sendpkt.c npf_session.c
SRCS+=		npf_state.c npf_tableset.c

.include <bsd.kmodule.mk>
