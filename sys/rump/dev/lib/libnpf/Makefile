#	$NetBSD: Makefile,v 1.1.2.2 2010/10/22 09:23:15 uebayasi Exp $
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
