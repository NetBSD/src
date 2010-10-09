# $NetBSD: Makefile,v 1.2.2.2 2010/10/09 03:32:36 yamt Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_ctl.c npf_handler.c npf_instr.c npf_mbuf.c
SRCS+=		npf_processor.c npf_ruleset.c npf_tableset.c npf_inet.c
SRCS+=		npf_session.c npf_nat.c npf_sendpkt.c npf_alg.c

.include <bsd.kmodule.mk>
