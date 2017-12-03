# $NetBSD: Makefile,v 1.10.2.5 2017/12/03 11:38:58 jdolecek Exp $
#
# Public Domain.
#

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf
IOCONF=		npf.ioconf
SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=		npf_bpf.c npf_if.c npf_inet.c npf_mbuf.c npf_nat.c
SRCS+=		npf_ruleset.c npf_conn.c npf_conndb.c npf_rproc.c
SRCS+=		npf_state.c npf_state_tcp.c npf_tableset.c
SRCS+=		lpm.c npf_sendpkt.c npf_worker.c npf_ifaddr.c npf_os.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
