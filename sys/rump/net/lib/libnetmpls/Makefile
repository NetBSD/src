# $NetBSD: Makefile,v 1.6 2022/09/03 20:29:30 thorpej Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls
COMMENT=Multiprotocol Label Switching (PF_MPLS)

IOCONF=	NETMPLS.ioconf
SRCS=	netmpls_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
