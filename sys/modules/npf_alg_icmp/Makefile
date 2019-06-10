# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:34 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_alg_icmp

SRCS=		npf_alg_icmp.c

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
