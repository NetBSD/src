# $NetBSD: Makefile,v 1.1.2.2 2011/11/10 14:31:50 yamt Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_alg_icmp

SRCS=		npf_alg_icmp.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
