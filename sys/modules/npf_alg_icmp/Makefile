# $NetBSD: Makefile,v 1.1.52.1 2018/09/30 01:45:55 pgoyette Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_alg_icmp

SRCS=		npf_alg_icmp.c

CPPFLAGS+=	-DINET6
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.include <bsd.kmodule.mk>
