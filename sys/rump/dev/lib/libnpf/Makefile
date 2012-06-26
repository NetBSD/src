#	$NetBSD: Makefile,v 1.1.18.1 2012/06/26 00:07:21 riz Exp $
#
# Public Domain.
#

.PATH:	${.CURDIR}/../../../../net/npf

LIB=	rumpdev_npf

SRCS=	npf.c npf_alg.c npf_ctl.c npf_handler.c
SRCS+=	npf_inet.c npf_instr.c npf_log.c npf_mbuf.c npf_nat.c
SRCS+=	npf_processor.c npf_ruleset.c npf_rproc.c npf_sendpkt.c
SRCS+=	npf_session.c npf_state.c npf_state_tcp.c npf_tableset.c

SRCS+=	component.c

WARNS=	4

CPPFLAGS+=	-D_NPF_TESTING
CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
