# $NetBSD: Makefile,v 1.1.4.2 2013/08/28 23:59:37 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../net ${.CURDIR}/../../../../netmpls

LIB=	rumpnet_netmpls

SRCS=	mpls_ttl.c if_mpls.c
SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/../libnet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
