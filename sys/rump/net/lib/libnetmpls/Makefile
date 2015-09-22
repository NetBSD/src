# $NetBSD: Makefile,v 1.2.10.1 2015/09/22 12:06:16 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls
IOCONF=	NETMPLS.ioconf
SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
