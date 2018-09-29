# $NetBSD: Makefile,v 1.2 2018/09/29 14:41:35 rmind Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_alg_icmp

SRCS=		npf_alg_icmp.c

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
