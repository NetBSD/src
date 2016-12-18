# $NetBSD: Makefile,v 1.17.2.1 2016/12/18 07:40:50 snj Exp $
#
# Public Domain.
#

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf

SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=		npf_bpf.c npf_if.c npf_inet.c npf_mbuf.c npf_nat.c
SRCS+=		npf_ruleset.c npf_conn.c npf_conndb.c npf_rproc.c
SRCS+=		npf_state.c npf_state_tcp.c npf_tableset.c
SRCS+=		lpm.c npf_sendpkt.c npf_worker.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
