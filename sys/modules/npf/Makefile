# $NetBSD: Makefile,v 1.15 2013/09/19 01:49:07 rmind Exp $
#
# Public Domain.
#

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c npf_bpf.c
SRCS+=		npf_inet.c npf_mbuf.c npf_nat.c npf_ruleset.c npf_rproc.c
SRCS+=		npf_sendpkt.c npf_session.c npf_state.c npf_state_tcp.c
SRCS+=		npf_tableset.c npf_tableset_ptree.c npf_worker.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
