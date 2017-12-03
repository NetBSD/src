# $NetBSD: Makefile,v 1.2.8.3 2017/12/03 11:39:19 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls
COMMENT=Multiprotocol Label Switching (PF_MPLS)

IOCONF=	NETMPLS.ioconf
SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
