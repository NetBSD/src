#	$NetBSD: Makefile,v 1.1.2.2 2013/02/25 00:30:10 tls Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net/npf

LIB=	rumpnet_npf

SRCS=	npf.c npf_alg.c npf_conf.c npf_ctl.c npf_handler.c
SRCS+=	npf_inet.c npf_instr.c npf_mbuf.c npf_nat.c
SRCS+=	npf_processor.c npf_ruleset.c npf_rproc.c npf_sendpkt.c
SRCS+=	npf_session.c npf_state.c npf_state_tcp.c
SRCS+=	npf_tableset.c npf_tableset_ptree.c

SRCS+=	npf_alg_icmp.c

SRCS+=	npf_ext_log.c npf_ext_normalise.c npf_ext_rndblock.c

SRCS+=	component.c

CPPFLAGS+=	-D_NPF_TESTING
CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
