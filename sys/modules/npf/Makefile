# $NetBSD: Makefile,v 1.1 2010/08/22 18:56:22 rmind Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_ctl.c npf_handler.c npf_instr.c npf_mbuf.c
SRCS+=		npf_processor.c npf_ruleset.c npf_tableset.c npf_inet.c
SRCS+=		npf_session.c npf_nat.c npf_alg.c

.include <bsd.kmodule.mk>
