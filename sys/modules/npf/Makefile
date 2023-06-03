# $NetBSD: Makefile,v 1.30 2023/06/03 08:52:58 lukem Exp $
#
# Public Domain.
#

.include "../Makefile.inc"

.PATH:		${S}/net/npf ${S}/external/bsd/libnv/dist/

KMOD=		npf
IOCONF=		npf.ioconf
SRCS=		npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=		npf_bpf.c npf_if.c npf_inet.c npf_mbuf.c npf_nat.c
SRCS+=		npf_params.c npf_ruleset.c npf_rproc.c
SRCS+=		npf_conn.c npf_conndb.c npf_connkey.c npf_portmap.c
SRCS+=		npf_state.c npf_state_tcp.c npf_tableset.c
SRCS+=		lpm.c npf_sendpkt.c npf_worker.c npf_os.c npf_ifaddr.c

SRCS+=		nvlist.c nvpair.c nv_kern_netbsd.c dnvlist.c

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I${S}/external/bsd/libnv/dist

COPTS.npf_inet.c+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}


.include <bsd.kmodule.mk>
