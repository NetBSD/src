#	$NetBSD: Makefile,v 1.1.8.2 2011/03/05 20:56:06 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../net/npf

LIB=	rumpdev_npf

SRCS=	npf.c npf_ctl.c npf_handler.c npf_instr.c npf_mbuf.c
SRCS+=	npf_processor.c npf_ruleset.c npf_tableset.c npf_inet.c
SRCS+=	npf_session.c npf_nat.c npf_alg.c

SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../../../librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
