# $NetBSD: Makefile,v 1.5 2015/10/19 16:16:38 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls
COMMENT=Multiprotocol Label Switching (PF_MPLS)

IOCONF=	NETMPLS.ioconf
SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
