# $NetBSD: Makefile,v 1.2.10.2 2015/12/27 12:10:17 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls
COMMENT=Multiprotocol Label Switching (PF_MPLS)

IOCONF=	NETMPLS.ioconf
SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
