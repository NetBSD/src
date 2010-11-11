# $NetBSD: Makefile,v 1.3 2010/11/11 06:30:39 rmind Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_ctl.c npf_handler.c npf_instr.c npf_mbuf.c
SRCS+=		npf_processor.c npf_ruleset.c npf_tableset.c npf_inet.c
SRCS+=		npf_session.c npf_state.c npf_nat.c npf_alg.c npf_sendpkt.c

.include <bsd.kmodule.mk>
