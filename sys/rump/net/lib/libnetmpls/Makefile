# $NetBSD: Makefile,v 1.2.4.2 2014/05/22 11:41:17 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls

SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	netmpls_component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
