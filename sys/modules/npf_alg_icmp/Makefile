# $NetBSD: Makefile,v 1.1 2011/11/06 13:26:54 tron Exp $

.include "../Makefile.inc"

.PATH:		${S}/net/npf

KMOD=		npf_alg_icmp

SRCS=		npf_alg_icmp.c

CPPFLAGS+=	-DINET6

.include <bsd.kmodule.mk>
