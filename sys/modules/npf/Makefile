# $NetBSD: Makefile,v 1.23 2019/02/17 04:05:55 rin Exp $
#
# Public Domain.
#

.include "../Makefile.inc"

.PATH:		${S}/net/npf ${S}/external/bsd/libnv/dist/

KMOD=		npf
IOCONF=		npf.ioconf
SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=		npf_bpf.c npf_if.c npf_inet.c npf_mbuf.c npf_nat.c
SRCS+=		npf_ruleset.c npf_conn.c npf_conndb.c npf_rproc.c
SRCS+=		npf_state.c npf_state_tcp.c npf_tableset.c
SRCS+=		lpm.c npf_sendpkt.c npf_worker.c npf_ifaddr.c npf_os.c
SRCS+=		nvlist.c nvpair.c nv_kern_netbsd.c dnvlist.c

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I${S}/external/bsd/libnv/dist

WARNS=		3

.include <bsd.kmodule.mk>
