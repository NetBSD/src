# $NetBSD: Makefile,v 1.2.8.2 2014/08/20 00:04:43 tls Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls

SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
