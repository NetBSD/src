# $NetBSD: Makefile,v 1.1 2013/07/18 15:59:28 kefren Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls

SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
